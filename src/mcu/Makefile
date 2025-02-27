# Define the compiler
CC = gcc

# Define the source files
MAIN_SRC = main.c

# Define the output executables
MAIN_OUT = main

# Define the build targets
all: $(MAIN_OUT)

$(MAIN_OUT): $(MAIN_SRC)
	$(CC) -o $(MAIN_OUT) $(MAIN_SRC)

# Clean up build artifacts
clean:
	rm -f $(MAIN_OUT)