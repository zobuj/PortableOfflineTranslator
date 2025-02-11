#!/bin/bash

cd whisper.cpp
make -j stream
./stream -m models/ggml-tiny.en.bin --step 4000 --length 8000 -c 0 -t 4 -ac 512