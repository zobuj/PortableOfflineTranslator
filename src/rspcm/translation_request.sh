#!/bin/bash

# Ensure a text argument is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 \"Text to translate\""
    exit 1
fi

TEXT="$1"

# Check if the pipeline process is running
pid=$(pgrep pipeline)
if [ -z "$pid" ]; then
    echo "Pipeline process not found!"
    exit 1
fi

echo "Triggering translation pipeline with text: \"$TEXT\""

# Activate Python virtual environment
source ../pcm_generator/venv/bin/activate

# Run Python script with text argument
python3 ../pcm_generator/generate_pcm_data.py --text "$TEXT" --mp3 ../pcm_generator/sample.mp3 --wav ../pcm_generator/sample.wav --pcm ../pcm_generator/sample.pcm

ffplay -f s24le -ar 24000 ../pcm_generator/sample.pcm

# Send signal to pipeline process
kill -SIGUSR1 "$pid"
