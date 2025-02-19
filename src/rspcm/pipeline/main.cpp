#include "../whisper.cpp/whisper.h"

#include <thread>
#include <string>
#include <iostream>
#include <unistd.h>
#include <time.h>

volatile bool start_translation = false;

void mcu_translate_signal_handler(int signum){
    if(signum == SIGUSR1) {
        start_translation = true;
    }
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
    std::string model     = "models/ggml-base.en.bin";
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