import subprocess
import argparse
import os

from gtts import gTTS

lang_map = {
    "en": "English", "zh": "Chinese", "de": "German", "es": "Spanish", "ru": "Russian",
    "ko": "Korean", "fr": "French", "ja": "Japanese", "pt": "Portuguese", "tr": "Turkish",
    "pl": "Polish", "ca": "Catalan", "nl": "Dutch", "ar": "Arabic", "sv": "Swedish",
    "it": "Italian", "id": "Indonesian", "hi": "Hindi", "fi": "Finnish", "vi": "Vietnamese"
}

def text_to_speech(mp3_filename, wav_filename, text, lang):
    """Generate speech using gTTS and convert to 24-bit WAV using FFmpeg"""
    try:
        tts = gTTS(text=text, lang=lang)
        tts.save(mp3_filename)

        # Convert MP3 to 24-bit PCM WAV using FFmpeg
        subprocess.run([
            "ffmpeg", "-i", mp3_filename, "-acodec", "pcm_s24le", 
            "-ac", "1", "-ar", "24000", wav_filename, "-y"
        ], check=True)

        if not os.path.exists(wav_filename):
            raise FileNotFoundError(f"FFmpeg failed to create {wav_filename}")

    except Exception as e:
        print(f"Error during text-to-speech conversion: {e}")
        exit(1)

def extract_pcm_from_wav(wav_filename, pcm_filename):
    """Extract raw 24-bit PCM data from a WAV file manually (bypassing wave module)"""
    try:
        with open(wav_filename, 'rb') as wav_file:
            wav_data = wav_file.read()

        # Find "data" chunk inside WAV header (for raw PCM extraction)
        data_index = wav_data.find(b'data')  
        if data_index == -1:
            raise ValueError("WAV file is malformed: 'data' chunk not found.")

        # The raw PCM data starts 8 bytes after the "data" marker
        pcm_data_start = data_index + 8
        pcm_data = wav_data[pcm_data_start:]

        # Save raw 24-bit PCM data
        with open(pcm_filename, 'wb') as pcm_file:
            pcm_file.write(pcm_data)

        print(f"Extracted PCM data: {len(pcm_data)} bytes (24-bit)")
        return pcm_data

    except Exception as e:
        print(f"Error processing WAV file: {e}")
        exit(1)

def setup_config(source_lang, dest_lang):
    """Write the source and destination language to config.txt"""
    try:
        with open("../pcm_generator/config.txt", "w") as file:
            file.write(f"{lang_map[source_lang]} {lang_map[dest_lang]}")
    except Exception as e:
        print(f"Error writing to config file: {e}")
        exit(1)

def main():
    """Main function to process text-to-speech and extract 24-bit PCM"""
    parser = argparse.ArgumentParser(description="Convert text to 24-bit PCM audio.")
    parser.add_argument("--text", type=str, required=True, help="Text to convert to speech.")
    parser.add_argument("--slang", type=str, default="en", help="Source language (default: en).")
    parser.add_argument("--dlang", type=str, default="es", help="Destination language (default: es).")
    parser.add_argument("--mp3", type=str, default="output.mp3", help="Output MP3 filename.")
    parser.add_argument("--wav", type=str, default="output.wav", help="Output WAV filename.")
    parser.add_argument("--pcm", type=str, default="output.pcm", help="Output PCM filename.")

    args = parser.parse_args()

    print(f"Generating speech: '{args.text}' (From: {args.slang} To: {args.dlang})")
    
    # Generate speech and convert to 24-bit PCM WAV
    text_to_speech(args.mp3, args.wav, args.text, args.slang)

    # Extract PCM data from WAV
    extract_pcm_from_wav(args.wav, args.pcm)

    # Set up config file
    setup_config(args.slang, args.dlang)

    print("Processing complete.")

if __name__ == "__main__":
    main()
