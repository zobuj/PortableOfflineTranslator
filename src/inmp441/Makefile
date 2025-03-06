CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Source files
CONSUMER_SRC = src/consumer.c
GENERATOR_SRC = src/generator.c

# Object files
CONSUMER_OBJ = obj/consumer.o
GENERATOR_OBJ = obj/generator.o

# Targets
CONSUMER_TARGET = build/consumer
GENERATOR_TARGET = build/generator

# Output files
OUTPUT_DIR = output
PCM_FILE = $(OUTPUT_DIR)/microphone_output.pcm
WAV_FILE = $(OUTPUT_DIR)/microphone_output.wav

# Default target
all: $(CONSUMER_TARGET) $(GENERATOR_TARGET)

# Build Consumer
$(CONSUMER_TARGET): $(CONSUMER_OBJ) | build
	$(CC) $(CFLAGS) $(CONSUMER_OBJ) -o $(CONSUMER_TARGET)

# Build Generator
$(GENERATOR_TARGET): $(GENERATOR_OBJ) | build
	$(CC) $(CFLAGS) $(GENERATOR_OBJ) -o $(GENERATOR_TARGET)

# Compile source files to object files
obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

# Create necessary directories
build:
	mkdir -p build

obj:
	mkdir -p obj

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Clean build files and output files
clean:
	rm -rf build obj $(OUTPUT_DIR)

# Run Consumer first, then Generator, then convert PCM to WAV
run: all $(OUTPUT_DIR)
	@echo "Starting I²S Consumer..."
	@$(CONSUMER_TARGET) $(PCM_FILE) & \
	consumer_pid=$$!; \
	sleep 1; \
	echo "Starting I²S Generator..."; \
	$(GENERATOR_TARGET); \
	wait $$consumer_pid; \
	echo "Converting PCM to WAV..."; \
	sox -t raw -r 48000 -c 2 -b 24 -e signed-integer $(PCM_FILE) $(WAV_FILE); \
	echo "Conversion complete! Output saved as $(WAV_FILE)"

