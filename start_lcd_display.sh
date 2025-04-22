#!/bin/bash

# === Start LCD Display ===
pushd src/user_interface/lcd
make
sudo ./lcd_display 2
popd