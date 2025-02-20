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
    bool no_timestamps        = true;

    std::string language  = "en";
    std::string model     = "whisper.cpp/models/ggml-tiny.en.bin";
    std::string fname_out = "";
};

// Whisper Print Usage Helper - stream.cpp
void whisper_print_usage(int argc, char ** argv, const whisper_params & params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help           show this help message and exit\n");
    fprintf(stderr, "  -s SEED,  --seed SEED      RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N,     --threads N      number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "            --step N         audio step size in milliseconds (default: %d)\n", params.step_ms);
    fprintf(stderr, "            --length N       audio length in milliseconds (default: %d)\n", params.length_ms);
    fprintf(stderr, "  -c ID,    --capture ID     capture device ID (default: -1)\n");
    fprintf(stderr, "  -mt N,    --max_tokens N   maximum number of tokens per audio chunk (default: %d)\n", params.max_tokens);
    fprintf(stderr, "  -ac N,    --audio_ctx N    audio context size (default: %d, 0 - all)\n", params.audio_ctx);
    fprintf(stderr, "  -su,      --speed-up       speed up audio by factor of 2 (faster processing, reduced accuracy, default: %s)\n", params.speed_up ? "true" : "false");
    fprintf(stderr, "  -v,       --verbose        verbose output\n");
    fprintf(stderr, "            --translate      translate from source language to english\n");
    fprintf(stderr, "  -kc,      --keep-context   keep text context from earlier audio (default: false)\n");
    fprintf(stderr, "  -ps,      --print_special  print special tokens\n");
    fprintf(stderr, "  -nt,      --no_timestamps  do not print timestamps\n");
    fprintf(stderr, "  -l LANG,  --language LANG  spoken language (default: %s)\n", params.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME    model path (default: %s)\n", params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME     text output file name (default: no output to file)\n");
    fprintf(stderr, "\n");
}

// Whisper Parameter Parser - stream.cpp
bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-s" || arg == "--seed") {
            params.seed = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--threads") {
            params.n_threads = std::stoi(argv[++i]);
        } else if (arg == "--step") {
            params.step_ms = std::stoi(argv[++i]);
        } else if (arg == "--length") {
            params.length_ms = std::stoi(argv[++i]);
        } else if (arg == "-c" || arg == "--capture") {
            params.capture_id = std::stoi(argv[++i]);
        } else if (arg == "-mt" || arg == "--max_tokens") {
            params.max_tokens = std::stoi(argv[++i]);
        } else if (arg == "-ac" || arg == "--audio_ctx") {
            params.audio_ctx = std::stoi(argv[++i]);
        } else if (arg == "-su" || arg == "--speed-up") {
            params.speed_up = true;
        } else if (arg == "-v" || arg == "--verbose") {
            params.verbose = true;
        } else if (arg == "--translate") {
            params.translate = true;
        } else if (arg == "-kc" || arg == "--keep-context") {
            params.no_context = false;
        } else if (arg == "-l" || arg == "--language") {
            params.language = argv[++i];
            if (whisper_lang_id(params.language.c_str()) == -1) {
                fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
                whisper_print_usage(argc, argv, params);
                exit(0);
            }
        } else if (arg == "-ps" || arg == "--print_special") {
            params.print_special_tokens = true;
        } else if (arg == "-nt" || arg == "--no_timestamps") {
            params.no_timestamps = true;
        } else if (arg == "-m" || arg == "--model") {
            params.model = argv[++i];
        } else if (arg == "-f" || arg == "--file") {
            params.fname_out = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void translate(){
    struct timespec start, end;
    double elapsed_time;

    fprintf(stdout, "Processing Translation Request...\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    
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
            return;
        }
        
        std::vector<int16_t> pcm16;
        int16_t sample;
        
        while (pcm_file.read(reinterpret_cast<char*>(&sample), sizeof(sample))) {
            pcm16.push_back(sample);
        }
        
        pcm_file.close();
        
        fprintf(stdout, "Read %zu samples from %s\n", pcm16.size(), pcm_filename.c_str());
    
        pcmf32.resize(pcm16.size());

        for (size_t i = 0; i < pcm16.size(); ++i) {
            pcmf32[i] = pcm16[i] / 32768.0f;  // Normalize to [-1,1]
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
            return;
        }

        // print result; - stream.cpp
        {
            printf("\r\33[2K");

            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const char * text = whisper_full_get_segment_text(ctx, i);

                if (params.no_timestamps) {
                    printf("%s\n", text);
                    fflush(stdout);
                } else {
                    const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                    const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

                    printf ("[%s --> %s]  %s\n", to_timestamp(t0).c_str(), to_timestamp(t1).c_str(), text);
                }
            }
            fflush(stdout); 
        }
    }




    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    fprintf(stdout, "Translation complete. Time taken: %.6f seconds.\n", elapsed_time);
    fprintf(stdout, "Returning to idle state.\n");
}

int main(int argc, char ** argv){
    fprintf(stdout, "Starting Translation Pipeline...\n");

    signal(SIGUSR1, mcu_translate_signal_handler);

    while(1){
        fprintf(stdout, "Waiting for translation request (send SIGUSR1 to PID %d)...\n", getpid());
        pause();
        if(start_translation) {
            translate();
            start_translation = false;
        }
    }

    return 0;
}