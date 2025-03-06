import pyaudio
import numpy as np
import struct

# Audio settings
FORMAT = pyaudio.paInt32  # PyAudio records in 32-bit (convert to 24-bit)
CHANNELS = 1  # Input is mono (since AirPods Pro doesn't support stereo)
OUTPUT_CHANNELS = 2  # Output is stereo
RATE = 48000  # Sample rate
CHUNK = 1024  # Buffer size
RECORD_SECONDS = 5  # Duration of recording
OUTPUT_FILENAME = "../inmp441/input/input_24bit_stereo.pcm"  # Raw PCM file

# Initialize PyAudio
audio = pyaudio.PyAudio()

# Open the audio stream
stream = audio.open(format=FORMAT, channels=CHANNELS,
                    rate=RATE, input=True,
                    frames_per_buffer=CHUNK)

print("Recording...")

frames = []

# Record the audio
for _ in range(int(RATE / CHUNK * RECORD_SECONDS)):
    data = stream.read(CHUNK)  # Prevent cut-off
    frames.append(data)

print("Recording finished.")

# Stop and close the stream
stream.stop_stream()
stream.close()
audio.terminate()

# Convert recorded data from 32-bit to proper 24-bit PCM
def convert_32bit_to_24bit_stereo(frames):
    audio_data = np.frombuffer(b''.join(frames), dtype=np.int32)  # Read as 32-bit
    audio_data = np.right_shift(audio_data, 8)  # Convert 32-bit to 24-bit

    audio_24bit = []

    # Pack into proper 24-bit PCM format with stereo duplication
    for sample in audio_data:
        packed_sample = struct.pack('<i', sample)[:3]  # Keep only 3 bytes (24-bit)
        audio_24bit.append(packed_sample * 2)  # Duplicate for stereo

    return b''.join(audio_24bit)

# Save as a raw PCM file (no headers, just pure audio data)
with open(OUTPUT_FILENAME, 'wb') as pcm_file:
    pcm_file.write(convert_32bit_to_24bit_stereo(frames))

print(f"Saved recording as {OUTPUT_FILENAME} (24-bit PCM Stereo)")
