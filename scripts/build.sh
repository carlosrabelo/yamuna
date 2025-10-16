#!/bin/bash
# Simple build script for YAMUNA

set -e

CORE_DIR="$(dirname "$0")/../core"

echo "Building YAMUNA firmware..."

if ! command -v pio &> /dev/null; then
    echo "Error: PlatformIO not found"
    echo "Install with: pip install platformio"
    exit 1
fi

cd "$CORE_DIR"

if pio run; then
    echo "Build successful!"
else
    echo "✗ Build failed!"
    exit 1
fi