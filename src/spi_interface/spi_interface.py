from gpiozero import Button
import spidev
import wave
import struct
import time
import os
import signal
import sys
from threading import Event

# Save our PID for the pipeline to find
pid_file = "/home/lorenzo/Documents/PortableOfflineTranslator/src/spi_interface/spi_interface.pid"
with open(pid_file, "w") as f:
    f.write(str(os.getpid()))
print(f"[SPI] Wrote PID {os.getpid()} to {pid_file}")


processing_done = Event()

def handle_sigusr1(signum, frame):
    print("\n[SPI] Received SIGUSR1 from pipeline process.")
    processing_done.set()

signal.signal(signal.SIGUSR1, handle_sigusr1)

# === Argument Parsing ===
if len(sys.argv) < 2:
    print("Usage: python3 spi_interface.py <pipeline_pid>")
    sys.exit(1)

try:
    PIPELINE_PID = int(sys.argv[1])
except ValueError:
    print("Invalid PID provided.")
    sys.exit(1)

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
STATE_PROCESSING = "processing"

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
            print(f"\rWaiting for interrupt from ESP32...", end='')
            time.sleep(0.1)

        elif state == STATE_POLLING:
            print(f"\rPolling for data...", end='')
            data = spi.xfer2([0x00] * CHUNK_SIZE)
            buffer.extend(data)
            time.sleep(max(0.01, CHUNK_DURATION - 0.005))
            print(f"\rReceived: {len(buffer)} bytes", end='')

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

            # time.sleep(5)

            # response_value = 0x1234
            # response_bytes = struct.pack('<H', response_value)
            # spi.xfer2(list(response_bytes))
            # print(f"Sent 0x{response_value:04X} back to ESP32")

            # Send Signal to Pipeline Process to start translation
            try:
                os.kill(PIPELINE_PID, signal.SIGUSR1)
                print(f"Signal SIGUSR1 sent to pipeline process PID {PIPELINE_PID}.")
            except ProcessLookupError:
                print(f"Error: No process found with PID {PIPELINE_PID}.")
            except PermissionError:
                print(f"Error: Permission denied to send signal to PID {PIPELINE_PID}.")

            state = STATE_PROCESSING
        elif state == STATE_PROCESSING:
            print("[SPI] Waiting for pipeline to finish (SIGUSR1)...")
            processing_done.clear()
            processing_done.wait()
            print("[SPI] Pipeline processing complete. Sending PCM data to ESP32...")

            time.sleep(5)

            # === Send Translated PCM Data ===
            TRANSLATED_PCM_PATH = "/home/lorenzo/Documents/PortableOfflineTranslator/src/rspcm/output/translated_output.pcm"
            try:
                with open(TRANSLATED_PCM_PATH, "rb") as pcm_file:
                    total_sent = 0
                    while True:
                        chunk = pcm_file.read(CHUNK_SIZE)
                        if not chunk:
                            break
                        if len(chunk) < CHUNK_SIZE:
                            chunk += b"\x00" * (CHUNK_SIZE - len(chunk))
                        spi.xfer2(list(chunk))
                        total_sent += len(chunk)
                        time.sleep(0.07)
                    print(f"[SPI] Sent total {total_sent} bytes from translated_output.pcm to ESP32")
            except FileNotFoundError:
                print(f"[SPI] Error: Translated PCM file not found at {TRANSLATED_PCM_PATH}")

            print("[SPI] Sending final response word to ESP32...")
            response_value = 0x1234
            response_bytes = struct.pack('<H', response_value)
            spi.xfer2(list(response_bytes))
            print(f"[SPI] Sent 0x{response_value:04X} back to ESP32")

            print("[SPI] Processing complete. Waiting for next interrupt...")
            state = STATE_WAITING








            response_value = 0x1234
            response_bytes = struct.pack('<H', response_value)
            spi.xfer2(list(response_bytes))
            print(f"Sent 0x{response_value:04X} back to ESP32")

            print("\rProcessing complete. Waiting for next interrupt...")
            state = STATE_WAITING
            

            

finally:
    if wav_file:
        wav_file.close()
    if pcm_file:
        pcm_file.close()
    spi.close()
