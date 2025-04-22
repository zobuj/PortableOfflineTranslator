from gpiozero import Button
import spidev
import wave
import struct
import time
import os
import signal

# === GPIO Interrupt Settings ===
INT_PIN = 17
button = Button(INT_PIN, pull_up=False)

# === SPI Settings ===
SPI_BUS = 0
SPI_DEVICE = 1
SPI_SPEED = 4_000_000 

# === Audio Settings ===
CHUNK_SIZE = 2048
# SAMPLE_RATE = 44100
SAMPLE_RATE = 16000
BITS_PER_SAMPLE = 16
BYTES_PER_SAMPLE = BITS_PER_SAMPLE // 8
CHANNELS = 1
WAV_OUTPUT_FILE = "recording_input.wav"
PCM_OUTPUT_FILE = "recording_input.pcm"
CHUNK_DURATION = CHUNK_SIZE / (SAMPLE_RATE * BYTES_PER_SAMPLE)

# === Init SPI ===
spi = spidev.SpiDev()
spi.open(SPI_BUS, SPI_DEVICE)
spi.max_speed_hz = SPI_SPEED
spi.mode = 0b00

# === State Definitions ===
STATE_WAITING = "waiting"
STATE_POLLING = "polling"
STATE_RESPONSE = "response"

state = STATE_WAITING

# === Global handles ===
wav_file = None
pcm_file = None
buffer = bytearray()

# === State Machine ===
def handle_interrupt():
    global state, wav_file, pcm_file, buffer

    if state == STATE_WAITING:
        print("\nInterrupt detected while waiting. Transitioning to polling state.")
        state = STATE_POLLING

        # === Setup WAV file ===
        print("Opening WAV and PCM files for recording...")
        buffer = bytearray()
        wav_file = wave.open(WAV_OUTPUT_FILE, 'wb')
        wav_file.setnchannels(CHANNELS)
        wav_file.setsampwidth(BYTES_PER_SAMPLE)
        wav_file.setframerate(SAMPLE_RATE)

        pcm_file = open(PCM_OUTPUT_FILE, 'wb')
        
    elif state == STATE_POLLING:
        print("\nInterrupt detected while polling. Transitioning to response state.")
        state = STATE_RESPONSE
    elif state == STATE_RESPONSE:
        print("\nInterrupt detected while processing response. Ignoring.")

button.when_pressed = handle_interrupt

try:
    while True:
        if state == STATE_WAITING:
            print(f"\rWaiting for interrupt from ESP32...", end='', flush=True)
            time.sleep(0.1)

        elif state == STATE_POLLING:
            print(f"\rPolling for data...", end='', flush=True)
            data = spi.xfer2([0x00] * CHUNK_SIZE)
            buffer.extend(data)
            time.sleep(max(0.01, CHUNK_DURATION - 0.005))
            print(f"\rReceived: {len(buffer)} bytes", end='', flush=True)

        elif state == STATE_RESPONSE:
            print("\rProcessing response...")

            if wav_file and pcm_file:
                wav_file.writeframes(buffer)
                pcm_file.write(buffer)
                wav_file.close()
                pcm_file.close()
                print(f"\nSaved {len(buffer)} bytes to {WAV_OUTPUT_FILE} and {PCM_OUTPUT_FILE}")
                wav_file = None
                pcm_file = None

            # Get Rid of Dummy Data
            spi.xfer2([0x00] * CHUNK_SIZE)

            time.sleep(5)

            response_value = 0x1234
            response_bytes = struct.pack('<H', response_value)
            spi.xfer2(list(response_bytes))
            print(f"Sent 0x{response_value:04X} back to ESP32")

            state = STATE_WAITING

finally:
    if wav_file:
        wav_file.close()
    if pcm_file:
        pcm_file.close()
    spi.close()
