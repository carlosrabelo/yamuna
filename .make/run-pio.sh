#!/usr/bin/env bash
set -euo pipefail

# Add default PlatformIO install path to PATH if pio is not in PATH
PIO_DEFAULT="$HOME/.platformio/penv/bin/pio"
if [ -f "$PIO_DEFAULT" ]; then
    export PATH="$(dirname "$PIO_DEFAULT"):$PATH"
fi

exec pio "$@"
