#include "../whisper.cpp/whisper.h"

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
    int32_t seed       = -1; // RNG seed, not used currently
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;

    bool speed_up             = false;
    bool verbose              = false;
    bool translate            = false;
    bool no_context           = true;
    bool print_special_tokens = false;
    bool no_timestamps        = true; // Sets the timestamps for the data

    std::string language  = "en";
    std::string model     = "whisper.cpp/models/ggml-tiny.en.bin";
    std::string fname_out = "";
};

std::string transcribe(){
    
    whisper_params params;

    struct whisper_context * ctx = whisper_init(params.model.c_str());

    const int n_samples = (params.step_ms/1000.0)*WHISPER_SAMPLE_RATE;
    const int n_samples_len = (params.length_ms/1000.0)*WHISPER_SAMPLE_RATE;
    const int n_samples_30s = 30*WHISPER_SAMPLE_RATE;
    const int n_samples_keep = 0.2*WHISPER_SAMPLE_RATE;

    std::vector<float> pcmf32;

    const int n_new_line = params.length_ms / params.step_ms - 1;

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
        fprintf(stderr, "%s: processing %d samples (step = %.1f sec / len = %.1f sec), %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                __func__,
                n_samples,
                float(n_samples)/WHISPER_SAMPLE_RATE,
                float(n_samples_len)/WHISPER_SAMPLE_RATE,
                params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);

        fprintf(stderr, "%s: n_new_line = %d\n", __func__, n_new_line);
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

        wparams.print_progress       = false;
        wparams.print_special_tokens = params.print_special_tokens;
        wparams.print_realtime       = false;
        wparams.print_timestamps     = !params.no_timestamps;
        wparams.translate            = params.translate;
        wparams.no_context           = params.no_context;
        wparams.single_segment       = true;
        wparams.max_tokens           = params.max_tokens;
        wparams.language             = params.language.c_str();
        wparams.n_threads            = params.n_threads;

        wparams.audio_ctx            = params.audio_ctx;
        wparams.speed_up             = params.speed_up;

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

                if (params.no_timestamps) {
                    // printf("%s\n", text);
                    result << text << " ";
                } else {
                    const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                    const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

                    // printf ("[%s --> %s]  %s\n", to_timestamp(t0).c_str(), to_timestamp(t1).c_str(), text);
                    result << "[" << to_timestamp(t0) << " --> " << to_timestamp(t1) << "] " << text << "\n";
                }
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