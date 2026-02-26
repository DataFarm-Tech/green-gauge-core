#!/bin/bash

# Check if NODE_ID argument was provided
if [ -z "$1" ]; then
    echo "Error: NODE_ID argument required"
    echo "Usage: $0 <NODE_ID>"
    exit 1
fi

NODE_ID="$1"

# Run the nvs_partition_gen.py command
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
    generate \
    "$NODE_ID/${NODE_ID}.csv" \
    "$NODE_ID/${NODE_ID}.bin" \
    0x6000