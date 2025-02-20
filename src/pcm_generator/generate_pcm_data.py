import subprocess
import wave
import argparse
from gtts import gTTS

def text_to_speech(mp3_filename, wav_filename, text):
    """Generate speech using gTTS and convert to 24-bit WAV using ffmpeg"""
    tts = gTTS(text=text, lang='en')
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

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Convert text to 24-bit PCM audio.")
    parser.add_argument("--text", type=str, required=True, help="Text to convert to speech.")
    parser.add_argument("--mp3", type=str, default="output.mp3", help="Output MP3 filename.")
    parser.add_argument("--wav", type=str, default="output.wav", help="Output WAV filename.")
    parser.add_argument("--pcm", type=str, default="output.pcm", help="Output PCM filename.")

    # Parse arguments
    args = parser.parse_args()

    # Generate speech and convert to WAV
    text_to_speech(args.mp3, args.wav, args.text)

    # Extract PCM data from WAV
    pcm_data = extract_pcm_from_wav(args.wav, args.pcm)

if __name__ == "__main__":
    main()