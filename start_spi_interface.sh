#!/bin/bash

# === Start SPI Interface ===
pushd src/spi_interface
python3 spi_interface.py &
sleep 1
popd
