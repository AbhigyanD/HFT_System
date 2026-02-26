#!/bin/bash
# NanoEX HFT System GUI launcher. Run from project root.

set -e

echo "NanoEX HFT System GUI"
echo "====================="

if [ ! -d "build" ]; then
    echo "Error: build directory not found. Run from project root."
    exit 1
fi

if [ ! -f "build/NanoEX HFT System.app/Contents/MacOS/NanoEX HFT System" ]; then
    echo "Error: GUI executable not found. Build with: cd build && cmake .. && make -j4"
    exit 1
fi

echo "Starting GUI."
cd build
exec "./NanoEX HFT System.app/Contents/MacOS/NanoEX HFT System"
