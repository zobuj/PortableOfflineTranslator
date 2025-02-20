import subprocess
import wave
import numpy as np
from gtts import gTTS

def text_to_speech(mp3_filename, wav_filename, text):
    """Generate speech using gTTS and convert to WAV using ffmpeg"""
    tts = gTTS(text=text, lang='en')
    tts.save(mp3_filename)

    # Convert MP3 to WAV using FFmpeg
    subprocess.run(["ffmpeg", "-i", mp3_filename, "-acodec", "pcm_s16le", "-ar", "24000", wav_filename, "-y"], check=True)

def extract_pcm_from_wav(wav_filename, pcm_filename):
    """Extract raw PCM data from a WAV file"""
    with wave.open(wav_filename, 'rb') as wav_file:
        num_channels = wav_file.getnchannels()
        sample_width = wav_file.getsampwidth()
        sample_rate = wav_file.getframerate()
        num_frames = wav_file.getnframes()
        pcm_data = wav_file.readframes(num_frames)  # Extract raw PCM bytes

    # Save raw PCM data
    with open(pcm_filename, 'wb') as pcm_file:
        pcm_file.write(pcm_data)

    print(f"Extracted PCM data: {len(pcm_data)} bytes")
    print(f"Channels: {num_channels}, Sample Width: {sample_width}, Sample Rate: {sample_rate}")

    return pcm_data

if __name__ == "__main__":
    text_to_speech('sample.mp3', 'sample.wav', 'No man, not at my house!')
    pcm_data = extract_pcm_from_wav('sample.wav', 'sample.pcm')
