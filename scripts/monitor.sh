#!/bin/bash
# Simple monitor script for YAMUNA

CORE_DIR="$(dirname "$0")/../core"

echo "Starting serial monitor..."
echo "Press Ctrl+C to exit"

if ! command -v pio &> /dev/null; then
    echo "Error: PlatformIO not found"
    echo "Install with: pip install platformio"
    exit 1
fi

cd "$CORE_DIR"
pio device monitor --baud 115200