#!/bin/bash

BINARY_PATH=/home/vansh/programs/sca/client

NUM_INSTANCES=10000

for ((i=1; i<=$NUM_INSTANCES; i++)); do
    $BINARY_PATH &  # Run the binary in the background
    echo "$i"
done

echo "All instances started."
