#!/bin/bash

# === Kill existing SPI interface instances ===
echo "Cleaning up existing spi_interface.py instances..."
pkill -f spi_interface.py

# === Build Pipeline ===
pushd src/rspcm/ > /dev/null
# ./rebuild.sh
cd build/
make
popd > /dev/null

# === Run Pipeline Executable (showing output) ===
PIPELINE_EXEC=src/rspcm/build/bin/pipeline
if [[ ! -f $PIPELINE_EXEC ]]; then
    echo "Error: Pipeline executable not found at $PIPELINE_EXEC"
    exit 1
fi

echo "Starting pipeline..."
$PIPELINE_EXEC &
PIPELINE_PID=$!
echo "Pipeline PID: $PIPELINE_PID"

# === Run SPI Interface Script (also showing output) ===
pushd src/spi_interface/ > /dev/null
echo "Starting SPI interface..."
python3 spi_interface.py "$PIPELINE_PID" &
SPI_PID=$!
popd > /dev/null

# === Wait for pipeline to finish ===
wait $PIPELINE_PID

# === Optional: Kill SPI interface when pipeline exits ===
kill $SPI_PID 2> /dev/null
