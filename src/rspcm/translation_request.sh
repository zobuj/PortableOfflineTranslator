#!/bin/bash
pid=$(pgrep pipeline)
if [ -z "$pid" ]; then
    echo "Pipeline process not found!"
    exit 1
fi

echo "Triggering translation pipeline..."
kill -SIGUSR1 "$pid"
