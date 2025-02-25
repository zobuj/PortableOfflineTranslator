#!/bin/bash

# Ensure text, slang, and dlang arguments are provided
if [ $# -lt 3 ]; then
    echo "Usage: $0 \"Text to translate\" slang dlang"
    exit 1
fi

TEXT="$1"
SLANG="$2"
DLANG="$3"

# Check if the pipeline process is running
pid=$(pgrep pipeline)
if [ -z "$pid" ]; then
    echo "Pipeline process not found!"
    exit 1
fi

echo "Triggering translation pipeline with text: \"$TEXT\", source language: \"$SLANG\", destination language: \"$DLANG\""

# Activate Python virtual environment
source ../pcm_generator/venv/bin/activate

# Run Python script with text, slang, and dlang arguments
python3 ../pcm_generator/generate_pcm_data.py --text "$TEXT" --mp3 ../pcm_generator/input.mp3 --wav ../pcm_generator/input.wav --pcm ../pcm_generator/input.pcm --slang "$SLANG" --dlang "$DLANG"

ffplay -f s24le -ar 24000 ../pcm_generator/input.pcm

# Send signal to pipeline process
kill -SIGUSR1 "$pid"

sleep 5

deactivate

source ../speaker/venv/bin/activate

tts --text "$(cat ../pcm_generator/translated_text.txt)" --model_name tts_models/es/css10/vits --out_path ../pcm_generator/output.wav && play ../pcm_generator/output.wav

