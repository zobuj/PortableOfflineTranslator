#include "../whisper.cpp/include/whisper.h"
#include "../llama.cpp/include/llama.h"

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
                // printf("%s\n", text);
                result << text << " ";
            }
        }
        return result.str();
    }
}

int translate(const std::string &text_in, std::string &text_out) {
    // Escape double quotes inside the input text
    std::string escaped_text = text_in;
    size_t pos = 0;
    while ((pos = escaped_text.find("\"", pos)) != std::string::npos) {
        escaped_text.replace(pos, 1, "\\\"");
        pos += 2;  // Move past the escaped quote
    }

    // path to the model gguf file
    std::string model_path = "llama.cpp/models/mistral-7b.Q4_K_M.gguf";
    // prompt to generate text from
    std::string prompt = "-p Translate the following text to Spanish: ' " + escaped_text + " ' --reverse-prompt Spanish:";
    // number of layers to offload to the GPU
    int ngl = 99;
    // number of tokens to predict
    int n_predict = 32;

    // load dynamic backends

    ggml_backend_load_all();

    // initialize the model

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = ngl;

    llama_model * model = llama_model_load_from_file(model_path.c_str(), model_params);
    const llama_vocab * vocab = llama_model_get_vocab(model);

    if (model == NULL) {
        fprintf(stderr , "%s: error: unable to load model\n" , __func__);
        return 1;
    }

    // tokenize the prompt

    // find the number of tokens in the prompt
    const int n_prompt = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, true, true);

    // allocate space for the tokens and tokenize the prompt
    std::vector<llama_token> prompt_tokens(n_prompt);
    if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true) < 0) {
        fprintf(stderr, "%s: error: failed to tokenize the prompt\n", __func__);
        return 1;
    }

    // initialize the context

    llama_context_params ctx_params = llama_context_default_params();
    // n_ctx is the context size
    ctx_params.n_ctx = n_prompt + n_predict - 1;
    // n_batch is the maximum number of tokens that can be processed in a single call to llama_decode
    ctx_params.n_batch = n_prompt;
    // enable performance counters
    ctx_params.no_perf = false;

    llama_context * ctx = llama_init_from_model(model, ctx_params);

    if (ctx == NULL) {
        fprintf(stderr , "%s: error: failed to create the llama_context\n" , __func__);
        return 1;
    }

    // initialize the sampler

    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = false;
    llama_sampler * smpl = llama_sampler_chain_init(sparams);

    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

    // print the prompt token-by-token

    for (auto id : prompt_tokens) {
        char buf[128];
        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
        if (n < 0) {
            fprintf(stderr, "%s: error: failed to convert token to piece\n", __func__);
            return 1;
        }
        std::string s(buf, n);
        printf("%s", s.c_str());
    }

    // prepare a batch for the prompt

    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

    // main loop

    const auto t_main_start = ggml_time_us();
    int n_decode = 0;
    llama_token new_token_id;

    std::string generated_text;  // String to store accumulated tokens

    for (int n_pos = 0; n_pos + batch.n_tokens < n_prompt + n_predict; ) {
        // evaluate the current batch with the transformer model
        if (llama_decode(ctx, batch)) {
            fprintf(stderr, "%s : failed to eval, return code %d\n", __func__, 1);
            return 1;
        }

        n_pos += batch.n_tokens;

        // sample the next token
        {
            new_token_id = llama_sampler_sample(smpl, ctx, -1);
        
            // is it an end of generation?
            if (llama_vocab_is_eog(vocab, new_token_id)) {
                break;
            }
        
            char buf[128];
            int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
            if (n < 0) {
                fprintf(stderr, "%s: error: failed to convert token to piece\n", __func__);
                return 1;
            }
        
            generated_text.append(buf, n);  // Append token to the accumulated string
        
            // prepare the next batch with the sampled token
            batch = llama_batch_get_one(&new_token_id, 1);
        
            n_decode += 1;
        }
        
        
    }

    printf("\n");

    text_out = generated_text;

    const auto t_main_end = ggml_time_us();

    fprintf(stderr, "%s: decoded %d tokens in %.2f s, speed: %.2f t/s\n",
            __func__, n_decode, (t_main_end - t_main_start) / 1000000.0f, n_decode / ((t_main_end - t_main_start) / 1000000.0f));

    fprintf(stderr, "\n");
    llama_perf_sampler_print(smpl);
    llama_perf_context_print(ctx);
    fprintf(stderr, "\n");

    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_model_free(model);

    return 0;
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
            std::string translated_text;
            translate(transcribed_text, translated_text);
            
            fprintf(stdout, "\nTranscribed text: \033[0;36m%s\033[0m\n\n", transcribed_text.c_str()); // Cyan
            fprintf(stdout, "Translated text: \033[0;32m%s\033[0m\n\n", translated_text.c_str()); // Green
            


            clock_gettime(CLOCK_MONOTONIC, &end);

            elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
            fprintf(stdout, "Translation complete. Time taken: %.6f seconds.\n", elapsed_time);
            fprintf(stdout, "Returning to idle state.\n");

            start_translation = false;
        }
    }

    return 0;
}