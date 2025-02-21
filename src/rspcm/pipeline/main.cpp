#include "../whisper.cpp/include/whisper.h"

#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <time.h>

volatile bool start_translation = false;

void mcu_translate_signal_handler(int signum){
    if(signum == SIGUSR1) {
        start_translation = true;
    }
}

//  500 -> 00:05.000
// 6000 -> 01:00.000
// To Timestamp Helper Function - stream.cpp
std::string to_timestamp(int64_t t) {
    int64_t sec = t/100;
    int64_t msec = t - sec*100;
    int64_t min = sec/60;
    sec = sec - min*60;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%03d", (int) min, (int) sec, (int) msec);

    return std::string(buf);
}

// Command-line Parameters - stream.cpp
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t keep_ms    = 200;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool translate     = false;
    bool no_fallback   = false;
    bool print_special = false;
    bool no_context    = true;
    bool no_timestamps = false;
    bool tinydiarize   = false;
    bool save_audio    = false; // save audio to wav file
    bool use_gpu       = true;
    bool flash_attn    = false;

    std::string language  = "en";
    std::string model     = "whisper.cpp/models/ggml-tiny.en.bin";
    std::string fname_out = "";
};

std::string transcribe(){
    
    whisper_params params;

    params.keep_ms   = std::min(params.keep_ms,   params.step_ms);
    params.length_ms = std::max(params.length_ms, params.step_ms);

    const int n_samples_step = (1e-3*params.step_ms  )*WHISPER_SAMPLE_RATE;
    const int n_samples_len  = (1e-3*params.length_ms)*WHISPER_SAMPLE_RATE;
    const int n_samples_keep = (1e-3*params.keep_ms  )*WHISPER_SAMPLE_RATE;
    const int n_samples_30s  = (1e-3*30000.0         )*WHISPER_SAMPLE_RATE;

    const bool use_vad = n_samples_step <= 0; // sliding window mode uses VAD

    const int n_new_line = !use_vad ? std::max(1, params.length_ms / params.step_ms - 1) : 1; // number of steps to print new line

    params.no_timestamps  = !use_vad;
    params.no_context    |= use_vad;
    params.max_tokens     = 0;

    // whisper init
    if (params.language != "auto" && whisper_lang_id(params.language.c_str()) == -1){
        fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
        exit(0);
    }

    struct whisper_context_params cparams = whisper_context_default_params();

    cparams.use_gpu    = params.use_gpu;
    cparams.flash_attn = params.flash_attn;

    struct whisper_context * ctx = whisper_init_from_file_with_params(params.model.c_str(), cparams);

    std::vector<float> pcmf32;

    // Processing Info - stream.cpp
    {
        fprintf(stderr, "\n");
        if (!whisper_is_multilingual(ctx)) {
            if (params.language != "en" || params.translate) {
                params.language = "en";
                params.translate = false;
                fprintf(stderr, "%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
            }
        }
        fprintf(stderr, "%s: processing %d samples (step = %.1f sec / len = %.1f sec / keep = %.1f sec), %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                __func__,
                n_samples_step,
                float(n_samples_step)/WHISPER_SAMPLE_RATE,
                float(n_samples_len )/WHISPER_SAMPLE_RATE,
                float(n_samples_keep)/WHISPER_SAMPLE_RATE,
                params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);

        if (!use_vad) {
            fprintf(stderr, "%s: n_new_line = %d, no_context = %d\n", __func__, n_new_line, params.no_context);
        } else {
            fprintf(stderr, "%s: using VAD, will transcribe on speech activity\n", __func__);
        }

        fprintf(stderr, "\n");
    }

    // Read PCM Data - Will need to change for I2C later
    {
        std::string pcm_filename = "../pcm_generator/sample.pcm";
        std::ifstream pcm_file(pcm_filename, std::ios::binary);
        
        if (!pcm_file) {
            fprintf(stderr, "Error: Unable to open %s!\n", pcm_filename.c_str());
            return "";
        }
        
        std::vector<uint8_t> pcm24_data;
        pcm24_data.resize(3);

        std::vector<int32_t> pcm24;
        
        while (pcm_file.read(reinterpret_cast<char*>(pcm24_data.data()), 3)) {
            // Reconstruct 24-bit sample (little-endian)
            int32_t sample = (pcm24_data[0] << 0) | (pcm24_data[1] << 8) | (pcm24_data[2] << 16);
    
            // Sign-extend to 32-bit
            if (sample & 0x800000) {
                sample |= 0xFF000000; // Fill upper 8 bits with 1s
            }
    
            pcm24.push_back(sample);
        }
        
        pcm_file.close();
        
        fprintf(stdout, "Read %zu samples from %s\n", pcm24.size(), pcm_filename.c_str());
    
        pcmf32.resize(pcm24.size());

        for (size_t i = 0; i < pcm24.size(); ++i) {
            // Normalize to [-1, 1]
            pcmf32[i] = static_cast<float>(pcm24[i]) / 8388608.0f;
        }

    }

    // Run the inference - stream.cpp
    {
        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

        wparams.print_progress   = false;
        wparams.print_special    = params.print_special;
        wparams.print_realtime   = false;
        wparams.print_timestamps = !params.no_timestamps;
        wparams.translate        = params.translate;
        wparams.single_segment   = !use_vad;
        wparams.max_tokens       = params.max_tokens;
        wparams.language         = params.language.c_str();
        wparams.n_threads        = params.n_threads;

        wparams.audio_ctx        = params.audio_ctx;

        wparams.tdrz_enable      = params.tinydiarize; // [TDRZ]
        wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;


        if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
            fprintf(stderr, "%s: failed to process audio\n", __func__);
            return "";
        }

        // print result; - stream.cpp (DEBUGGING)
        std::ostringstream result;

        {
            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const char * text = whisper_full_get_segment_text(ctx, i);
                printf("%s\n", text);
                result << text << " ";
            }
        }
        return result.str();
    }
}

void translate(const std::string &text) {
    // Escape double quotes inside the input text
    std::string escaped_text = text;
    size_t pos = 0;
    while ((pos = escaped_text.find("\"", pos)) != std::string::npos) {
        escaped_text.replace(pos, 1, "\\\"");
        pos += 2;  // Move past the escaped quote
    }

    // Construct the command dynamically
    std::string command = "./llama.cpp/build/bin/llama-simple -m llama.cpp/models/mistral-7b.Q4_K_M.gguf "
                          "-p \"Translate the following text to French: '" + escaped_text + "'\" "
                          "--reverse-prompt \"French:\"";

    // Execute command
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "Error: Translation script failed with exit code " << result << std::endl;
    }

}


int main(int argc, char ** argv){
    fprintf(stdout, "Starting Translation Pipeline...\n");
    signal(SIGUSR1, mcu_translate_signal_handler);
    while(1){
        fprintf(stdout, "Waiting for translation request (send SIGUSR1 to PID %d)...\n", getpid());
        fprintf(stdout, "Press Ctrl+C to exit.\n");
        pause();
        if(start_translation) {

            struct timespec start, end;
            double elapsed_time;
        
            fprintf(stdout, "Processing Translation Request...\n");
        
            clock_gettime(CLOCK_MONOTONIC, &start);

            std::string transcribed_text = transcribe();
            translate(transcribed_text);


            clock_gettime(CLOCK_MONOTONIC, &end);

            elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
            fprintf(stdout, "Translation complete. Time taken: %.6f seconds.\n", elapsed_time);
            fprintf(stdout, "Returning to idle state.\n");

            start_translation = false;
        }
    }

    return 0;
}