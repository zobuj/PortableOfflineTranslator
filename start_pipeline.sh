#!/bin/bash

# === Build Pipeline ===
pushd src/rspcm/ > /dev/null
cd build/
make
popd > /dev/null

# === Run Pipeline with Logging ===
PIPELINE_EXEC=src/rspcm/build/bin/pipeline
if [[ ! -f $PIPELINE_EXEC ]]; then
    echo "Error: Pipeline executable not found at $PIPELINE_EXEC"
    exit 1
fi

MEM_LOG="output/pipeline_mem_log.csv"
mkdir -p output
echo "timestamp,rss_kb,vsz_kb" > $MEM_LOG

# Run pipeline in a subshell so we can track its PID and wait on it
(
    $PIPELINE_EXEC &
    PIPELINE_PID=$!

    # Memory logging loop
    while kill -0 $PIPELINE_PID 2>/dev/null; do
        TIMESTAMP=$(date +%s)
        MEM=$(ps -o rss=,vsz= -p $PIPELINE_PID)
        echo "$TIMESTAMP,$MEM" >> $MEM_LOG
        sleep 0.01
    done

    wait $PIPELINE_PID
)
