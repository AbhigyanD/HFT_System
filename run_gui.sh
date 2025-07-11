#!/bin/bash

# NanoEX HFT System GUI Launcher
# This script launches the visual interface for the momentum strategy

echo "🚀 Launching NanoEX HFT System GUI..."
echo "====================================="

# Check if we're in the right directory
if [ ! -d "build" ]; then
    echo "❌ Error: build directory not found!"
    echo "Please run this script from the project root directory."
    exit 1
fi

# Check if the GUI executable exists
if [ ! -f "build/NanoEX HFT System.app/Contents/MacOS/NanoEX HFT System" ]; then
    echo "❌ Error: GUI executable not found!"
    echo "Please build the project first with:"
    echo "  cd build && cmake .. && make -j4"
    exit 1
fi

# Launch the GUI
echo "✅ Starting visual interface..."
echo "📊 You should see a window with tabs for:"
echo "   - Strategy Chart (real-time visualization)"
echo "   - Strategy Signals (live signal feed)"
echo "   - Strategy Config (current settings)"
echo "   - Performance (system metrics)"
echo "   - System Output (complete log)"
echo ""
echo "🎮 Click 'Start HFT System' to begin!"
echo ""

cd build
./NanoEX\ HFT\ System.app/Contents/MacOS/NanoEX\ HFT\ System 