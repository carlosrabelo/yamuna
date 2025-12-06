#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_FILE="$ROOT_DIR/.env"

for port in /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyUSB2 /dev/ttyACM0; do
    if [ -e "$port" ]; then
        if [ -f "$ENV_FILE" ]; then
            sed -i "s|^UPLOAD_PORT=.*|UPLOAD_PORT=$port|" "$ENV_FILE"
            sed -i "s|^MONITOR_PORT=.*|MONITOR_PORT=$port|" "$ENV_FILE"
        else
            echo "UPLOAD_PORT=$port" > "$ENV_FILE"
            echo "MONITOR_PORT=$port" >> "$ENV_FILE"
            echo "MONITOR_SPEED=115200" >> "$ENV_FILE"
            echo "UPLOAD_SPEED=921600" >> "$ENV_FILE"
        fi
        echo "Detected board at: $port (saved to .env)"
        exit 0
    fi
done

echo "No board detected on USB ports."
echo "Checked: /dev/ttyUSB0, /dev/ttyUSB1, /dev/ttyUSB2, /dev/ttyACM0"
exit 1
