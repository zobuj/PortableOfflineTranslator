# from gpiozero import Button
# import spidev
# import time

# INT_PIN = 17
# spi = spidev.SpiDev()
# spi.open(0, 0)
# spi.max_speed_hz = 1_000_000

# button = Button(INT_PIN, pull_up=False)

# def read_audio():
#     data = spi.xfer2([0x00] * 1024)
#     print("Received:", data[:16])

# button.when_pressed = read_audio

# print("Waiting for audio data...")
# while True:
#     time.sleep(0.01)


import spidev
import wave
import time
from datetime import datetime
import struct

# === SPI Settings ===
SPI_BUS = 0
SPI_DEVICE = 0
SPI_SPEED = 4_000_000  # Try a higher speed for cleaner timing

# === Audio Settings ===
CHUNK_SIZE = 2048  # Must match ESP32 buffer

SAMPLE_RATE = 44100
# SAMPLE_RATE = 16000
BITS_PER_SAMPLE = 16
BYTES_PER_SAMPLE = BITS_PER_SAMPLE // 8
CHANNELS = 1
RECORD_SECONDS = 5
OUTPUT_FILE = f"recording_input.wav"
CHUNK_DURATION = CHUNK_SIZE / (SAMPLE_RATE * BYTES_PER_SAMPLE)

# === Derived values ===
TARGET_BYTES = SAMPLE_RATE * BYTES_PER_SAMPLE * RECORD_SECONDS

# === Init SPI ===
spi = spidev.SpiDev()
spi.open(SPI_BUS, SPI_DEVICE)
spi.max_speed_hz = SPI_SPEED
spi.mode = 0b00

print(f"Recording {RECORD_SECONDS} sec to {OUTPUT_FILE}...")

# === WAV File Setup ===
wav_file = wave.open(OUTPUT_FILE, 'wb')
wav_file.setnchannels(CHANNELS)
wav_file.setsampwidth(BYTES_PER_SAMPLE)
wav_file.setframerate(SAMPLE_RATE)

# === Record Loop ===
buffer = bytearray()
start = time.time()

try:
    while len(buffer) < TARGET_BYTES:
        to_read = min(CHUNK_SIZE, TARGET_BYTES - len(buffer))
        data = spi.xfer2([0x00] * to_read)
        buffer.extend(data)
        print(f"\rReceived: {len(buffer)} / {TARGET_BYTES} bytes", end='', flush=True)

        time.sleep(CHUNK_DURATION)

    print("\n\nDecoded first 16 samples:")
    for i in range(0, 32, 2):
        sample = struct.unpack('<h', buffer[i:i+2])[0]
        print(f"Sample {i//2}: {sample}")

    print("Done. Writing to WAV...")
    wav_file.writeframes(buffer)
    print(f"Saved {len(buffer)} bytes to {OUTPUT_FILE}")

finally:
    wav_file.close()
    spi.close()
