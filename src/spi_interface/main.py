from gpiozero import Button
import spidev
import wave
import struct
import time

# === GPIO Interrupt Settings ===
INT_PIN = 17
button = Button(INT_PIN, pull_up=False)

# === SPI Settings ===
SPI_BUS = 0
SPI_DEVICE = 1
SPI_SPEED = 4_000_000 

# === Audio Settings ===
CHUNK_SIZE = 2048  # Must match ESP32 buffer - 2048 bytes
SAMPLE_RATE = 44100
BITS_PER_SAMPLE = 16
BYTES_PER_SAMPLE = BITS_PER_SAMPLE // 8
CHANNELS = 1
OUTPUT_FILE = f"recording_input.wav"
CHUNK_DURATION = CHUNK_SIZE / (SAMPLE_RATE * BYTES_PER_SAMPLE) # 2048 bytes / (44100 samples/sec * 2 bytes/sample) = 0.0232 sec

# === Init SPI ===
spi = spidev.SpiDev()
spi.open(SPI_BUS, SPI_DEVICE)
spi.max_speed_hz = SPI_SPEED
spi.mode = 0b00

# === State Definitions ===
STATE_WAITING = "waiting"
STATE_POLLING = "polling"
STATE_RESPONSE = "response"

# === Initial State ===
state = STATE_WAITING

# === Global WAV handle ===
wav_file = None

# === Global Data Buffer ===
buffer = bytearray()

# === State Machine ===
def handle_interrupt():
    global state, wav_file, buffer

    if state == STATE_WAITING:
        print("\nInterrupt detected while waiting. Transitioning to polling state.")
        state = STATE_POLLING

        # === Setup WAV file ===
        print("Opening WAV file for recording...")
        buffer = bytearray()  # clear buffer just in case
        wav_file = wave.open(OUTPUT_FILE, 'wb')
        wav_file.setnchannels(CHANNELS)
        wav_file.setsampwidth(BYTES_PER_SAMPLE)
        wav_file.setframerate(SAMPLE_RATE)
        
    elif state == STATE_POLLING:
        print("\nInterrupt detected while polling. Transitioning to response state.")
        state = STATE_RESPONSE
    elif state == STATE_RESPONSE:
        print("\nInterrupt detected while processing response. Ignoring.")

button.when_pressed = handle_interrupt

try:
    while True:
        if state == STATE_WAITING:
            # Waiting for interrupt
            print(f"\rWaiting for interrupt from ESP32...", end='', flush=True)
            time.sleep(0.1)

        elif state == STATE_POLLING:
            # Polling for data
            print(f"\rPolling for data...", end='', flush=True)

            data = spi.xfer2([0x00] * CHUNK_SIZE)
            buffer.extend(data)
            # time.sleep(CHUNK_DURATION)  # wait to match data rate
            time.sleep(max(0.01, CHUNK_DURATION - 0.005))

            print(f"\rReceived: {len(buffer)} bytes", end='', flush=True)


        elif state == STATE_RESPONSE:
            # Handle response (e.g., save to file)
            print("\rProcessing response...")

            if wav_file:
                wav_file.writeframes(buffer)
                wav_file.close()
                print(f"\nSaved {len(buffer)} bytes to {OUTPUT_FILE}")
                wav_file = None


            # === Get Rid of Dummy Data ===
            dummy_data = spi.xfer2([0x00] * CHUNK_SIZE)

            time.sleep(5)  # Give some time before sending response

            # === Send 16-bit value back to ESP32 ===
            response_value = 0x1234  # Replace this with your actual value
            response_bytes = struct.pack('<H', response_value)
            spi.xfer2(list(response_bytes))
            print(f"Sent 0x{response_value:04X} back to ESP32")

            state = STATE_WAITING

finally:
    spi.close()