#!/bin/bash
# Simple flash script for YAMUNA

set -e

CORE_DIR="$(dirname "$0")/../core"

echo "Flashing YAMUNA firmware..."

if ! command -v pio &> /dev/null; then
    echo "Error: PlatformIO not found"
    echo "Install with: pip install platformio"
    exit 1
fi

cd "$CORE_DIR"

# Build first if needed
if [ ! -f ".pio/build/esp32-release/firmware.bin" ]; then
    echo "Building firmware first..."
    pio run
fi

if pio run --target upload; then
    echo "Flash successful!"
else
    echo "✗ Flash failed!"
    exit 1
fi