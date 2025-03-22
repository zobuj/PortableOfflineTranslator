import subprocess
import wave
import argparse
from gtts import gTTS

lang_map = {
    "en": "English",
    "zh": "Chinese",
    "de": "German",
    "es": "Spanish",
    "ru": "Russian",
    "ko": "Korean",
    "fr": "French",
    "ja": "Japanese",
    "pt": "Portuguese",
    "tr": "Turkish",
    "pl": "Polish",
    "ca": "Catalan",
    "nl": "Dutch",
    "ar": "Arabic",
    "sv": "Swedish",
    "it": "Italian",
    "id": "Indonesian",
    "hi": "Hindi",
    "fi": "Finnish",
    "vi": "Vietnamese",
    "he": "Hebrew",
    "uk": "Ukrainian",
    "el": "Greek",
    "ms": "Malay",
    "cs": "Czech",
    "ro": "Romanian",
    "da": "Danish",
    "hu": "Hungarian",
    "ta": "Tamil",
    "no": "Norwegian",
    "th": "Thai",
    "ur": "Urdu",
    "hr": "Croatian",
    "bg": "Bulgarian",
    "lt": "Lithuanian",
    "la": "Latin",
    "mi": "Maori",
    "ml": "Malayalam",
    "cy": "Welsh",
    "sk": "Slovak",
    "te": "Telugu",
    "fa": "Persian",
    "lv": "Latvian",
    "bn": "Bengali",
    "sr": "Serbian",
    "az": "Azerbaijani",
    "sl": "Slovenian",
    "kn": "Kannada",
    "et": "Estonian",
    "mk": "Macedonian",
    "br": "Breton",
    "eu": "Basque",
    "is": "Icelandic",
    "hy": "Armenian",
    "ne": "Nepali",
    "mn": "Mongolian",
    "bs": "Bosnian",
    "kk": "Kazakh",
    "sq": "Albanian",
    "sw": "Swahili",
    "gl": "Galician",
    "mr": "Marathi",
    "pa": "Punjabi",
    "si": "Sinhala",
    "km": "Khmer",
    "sn": "Shona",
    "yo": "Yoruba",
    "so": "Somali",
    "af": "Afrikaans",
    "oc": "Occitan",
    "ka": "Georgian",
    "be": "Belarusian",
    "tg": "Tajik",
    "sd": "Sindhi",
    "gu": "Gujarati",
    "am": "Amharic",
    "yi": "Yiddish",
    "lo": "Lao",
    "uz": "Uzbek",
    "fo": "Faroese",
    "ht": "Haitian Creole",
    "ps": "Pashto",
    "tk": "Turkmen",
    "nn": "Nynorsk",
    "mt": "Maltese",
    "sa": "Sanskrit",
    "lb": "Luxembourgish",
    "my": "Myanmar",
    "bo": "Tibetan",
    "tl": "Tagalog",
    "mg": "Malagasy",
    "as": "Assamese",
    "tt": "Tatar",
    "haw": "Hawaiian",
    "ln": "Lingala",
    "ha": "Hausa",
    "ba": "Bashkir",
    "jw": "Javanese",
    "su": "Sundanese",
    "yue": "Cantonese"
}


def text_to_speech(mp3_filename, wav_filename, text, lang):
    """Generate speech using gTTS and convert to 24-bit WAV using ffmpeg"""
    tts = gTTS(text=text, lang=lang)
    tts.save(mp3_filename)

    # Convert MP3 to 24-bit WAV using FFmpeg
    subprocess.run(["ffmpeg", "-i", mp3_filename, "-acodec", "pcm_s24le", "-ar", "24000", wav_filename, "-y"], check=True)

def extract_pcm_from_wav(wav_filename, pcm_filename):
    """Extract raw 24-bit PCM data from a WAV file"""
    with wave.open(wav_filename, 'rb') as wav_file:
        num_channels = wav_file.getnchannels()
        sample_width = wav_file.getsampwidth()  # Should now be 3 bytes (24-bit)
        sample_rate = wav_file.getframerate()
        num_frames = wav_file.getnframes()

        pcm_data = wav_file.readframes(num_frames)  # Extract raw PCM bytes

    # Ensure we're handling 24-bit (3 bytes per sample)
    if sample_width != 3:
        raise ValueError(f"Expected 24-bit audio (3-byte samples), but got {sample_width}-byte samples.")

    # Save raw 24-bit PCM data
    with open(pcm_filename, 'wb') as pcm_file:
        pcm_file.write(pcm_data)

    print(f"Extracted PCM data: {len(pcm_data)} bytes")
    print(f"Channels: {num_channels}, Sample Width: {sample_width} bytes, Sample Rate: {sample_rate}")

    return pcm_data

def setup_config(source_lang, dest_lang):
    with open("../pcm_generator/config.txt", "w") as file:
        file.write(f"{lang_map[source_lang]} {lang_map[dest_lang]}")

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Convert text to 24-bit PCM audio.")
    parser.add_argument("--text", type=str, required=True, help="Text to convert to speech.")
    parser.add_argument("--slang", type=str, default="en", help="Language code (default: en).")
    parser.add_argument("--dlang", type=str, default="es", help="Language code (default: es).")
    parser.add_argument("--mp3", type=str, default="output.mp3", help="Output MP3 filename.")
    parser.add_argument("--wav", type=str, default="output.wav", help="Output WAV filename.")
    parser.add_argument("--pcm", type=str, default="output.pcm", help="Output PCM filename.")

    # Parse arguments
    args = parser.parse_args()

    # # Generate speech and convert to WAV
    # text_to_speech(args.mp3, args.wav, args.text, args.slang)

    # # Extract PCM data from WAV
    # pcm_data = extract_pcm_from_wav(args.wav, args.pcm)

    # Print the source and destination languages
    print(f"Source Language: {args.slang}")
    print(f"Destination Language: {args.dlang}")
    # Set up config file
    setup_config(args.slang, args.dlang)


if __name__ == "__main__":
    main()