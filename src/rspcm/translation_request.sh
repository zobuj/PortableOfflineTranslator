#!/bin/bash

set -e  # Exit on error
set -u  # Exit if an unset variable is used
set -o pipefail  # Exit if a pipeline command fails

# Function to print usage instructions
usage() {
    echo "Usage: $0 -t \"Text to translate\" -s source_lang -d dest_lang"
    exit 1
}

# Parse command-line arguments
while getopts ":t:s:d:" opt; do
    case "${opt}" in
        t) TEXT=${OPTARG} ;;
        s) SLANG=${OPTARG} ;;
        d) DLANG=${OPTARG} ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

# Ensure required arguments are provided
if [[ -z "${TEXT:-}" || -z "${SLANG:-}" || -z "${DLANG:-}" ]]; then
    usage
fi

# Function to select the correct TTS model based on destination language
select_tts_model() {
    case "$1" in
        en) echo "tts_models/en/ljspeech/tacotron2-DDC" ;;
        es) echo "tts_models/es/css10/vits" ;;
        fr) echo "tts_models/fr/css10/vits" ;;
        de) echo "tts_models/de/thorsten/vits" ;;
        it) echo "tts_models/it/mai_female/vits" ;;
        pt) echo "tts_models/pt/cv/vits" ;;
        zh) echo "tts_models/zh-CN/baker/tacotron2-DDC-GST" ;;
        ja) echo "tts_models/ja/kokoro/tacotron2-DDC" ;;
        nl) echo "tts_models/nl/css10/vits" ;;
        tr) echo "tts_models/tr/common-voice/glow-tts" ;;
        uk) echo "tts_models/uk/mai/vits" ;;
        ca) echo "tts_models/ca/custom/vits" ;;
        fa) echo "tts_models/fa/custom/glow-tts" ;;
        bg) echo "tts_models/bg/cv/vits" ;;
        cs) echo "tts_models/cs/cv/vits" ;;
        da) echo "tts_models/da/cv/vits" ;;
        et) echo "tts_models/et/cv/vits" ;;
        ga) echo "tts_models/ga/cv/vits" ;;
        el) echo "tts_models/el/cv/vits" ;;
        fi) echo "tts_models/fi/css10/vits" ;;
        hr) echo "tts_models/hr/cv/vits" ;;
        lt) echo "tts_models/lt/cv/vits" ;;
        lv) echo "tts_models/lv/cv/vits" ;;
        mt) echo "tts_models/mt/cv/vits" ;;
        pl) echo "tts_models/pl/mai_female/vits" ;;
        ro) echo "tts_models/ro/cv/vits" ;;
        sk) echo "tts_models/sk/cv/vits" ;;
        sl) echo "tts_models/sl/cv/vits" ;;
        sv) echo "tts_models/sv/cv/vits" ;;
        be) echo "tts_models/be/common-voice/glow-tts" ;;
        hu) echo "tts_models/hu/css10/vits" ;;
        bn) echo "tts_models/bn/custom/vits-female" ;;
        ewe) echo "tts_models/ewe/openbible/vits" ;;
        hau) echo "tts_models/hau/openbible/vits" ;;
        lin) echo "tts_models/lin/openbible/vits" ;;
        tw_akuapem) echo "tts_models/tw_akuapem/openbible/vits" ;;
        tw_asante) echo "tts_models/tw_asante/openbible/vits" ;;
        yor) echo "tts_models/yor/openbible/vits" ;;
        *) echo "Error: No TTS model found for language '$1'"; exit 1 ;;
    esac
}

# Get the correct TTS model
TTS_MODEL=$(select_tts_model "$DLANG")

# Locate pipeline process
PID=$(pgrep pipeline || true)
if [[ -z "$PID" ]]; then
    echo "Error: Pipeline process not found!"
    exit 1
fi

echo "Triggering translation pipeline: \"$TEXT\" (From: \"$SLANG\" To: \"$DLANG\")"

# Activate Python virtual environment for PCM generation
PCM_VENV_PATH="../pcm_generator/venv/bin/activate"
if [[ ! -f "$PCM_VENV_PATH" ]]; then
    echo "Error: Python virtual environment not found at $PCM_VENV_PATH"
    exit 1
fi

source "$PCM_VENV_PATH"

# Run the PCM generation script
python3 ../pcm_generator/generate_pcm_data_rspi.py \
    --text "$TEXT" \
    --mp3 ../pcm_generator/input/input.mp3 \
    --wav ../pcm_generator/input/input.wav \
    --pcm ../pcm_generator/input/input.pcm \
    --slang "$SLANG" \
    --dlang "$DLANG"

deactivate  # Deactivate PCM virtual environment

# # Play the PCM file
# ffplay -nodisp -autoexit -f s24le -ar 24000 ../pcm_generator/input.pcm

# Send signal to pipeline process
pushd ../mcu
make clean
make mcu
popd
../mcu/mcu

sleep 90  # Allow pipeline process to process the signal

# # Activate Python virtual environment for TTS
# SPEAKER_VENV_PATH="../speaker/venv/bin/activate"
# if [[ ! -f "$SPEAKER_VENV_PATH" ]]; then
#     echo "Error: Python virtual environment not found at $SPEAKER_VENV_PATH"
#     exit 1
# fi

# source "$SPEAKER_VENV_PATH"

pushd piper/

# Ensure translated text file exists
TRANSLATED_TEXT_PATH="../../pcm_generator/output/translated_text.txt"
if [[ ! -f "$TRANSLATED_TEXT_PATH" ]]; then
    echo "Error: Translated text file not found at $TRANSLATED_TEXT_PATH"
    deactivate
    exit 1
fi

# # Run TTS with the correct model
# tts --text "$(cat "$TRANSLATED_TEXT_PATH")" --model_name "$TTS_MODEL" --out_path ../pcm_generator/output.wav
# # Remove the trimmed output file if it exists
# TRIMMED_OUTPUT_PATH="../pcm_generator/trimmed_output.wav"
# if [[ -f "$TRIMMED_OUTPUT_PATH" ]]; then
#     rm "$TRIMMED_OUTPUT_PATH"
# fi
# ffmpeg -i ../pcm_generator/output.wav -af silenceremove=stop_periods=1:stop_duration=0.5:stop_threshold=-40dB ../pcm_generator/trimmed_output.wav

# play ../pcm_generator/trimmed_output.wav

# deactivate  # Deactivate speaker virtual environment

# cat "$TRANSLATED_TEXT_PATH" | ./piper --model en_US-lessac-medium.onnx --output_file ../../pcm_generator/output/translated_output.wav
# cat "$TRANSLATED_TEXT_PATH" | ./piper --model es_ES-carlfm-x_low.onnx --output_file ../../pcm_generator/output/translated_output.wav
# cat "$TRANSLATED_TEXT_PATH" | ./piper --model it_IT-paola-medium.onnx --output_file ../../pcm_generator/output/translated_output.wav
cat "$TRANSLATED_TEXT_PATH" | ./piper --model zh_CN-huayan-x_low.onnx --output_file ../../pcm_generator/output/translated_output.wav

popd


echo "Translation and speech synthesis completed successfully."
