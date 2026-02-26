# NanoEX HFT System GUI Installation Guide

This guide will help you install and run the GUI version of the NanoEX High-Frequency Trading System.

## GUI Features

The GUI application provides:
- **Real-time System Control**: Start/stop the HFT system with a single click
- **Live Performance Monitoring**: Real-time display of orders/sec, latency, and trades
- **Strategy Performance Table**: Live updates of strategy metrics
- **System Output Display**: Real-time console output with auto-scrolling
- **Log Management**: Save and view system logs
- **Professional Interface**: Clean, modern Qt-based GUI

## Prerequisites

### Required Software
1. **C++ Compiler**: GCC 7+ or Clang 5+ with C++17 support
2. **Qt Framework**: Qt5 or Qt6 (for GUI)
3. **CMake**: Version 3.16+ (optional, for easier building)

### macOS Installation

#### Option 1: Using Homebrew (Recommended)
```bash
# Install Qt6
brew install qt6

# Install CMake (if not already installed)
brew install cmake

# Verify installation
qmake --version
cmake --version
```

#### Option 2: Using Qt Official Installer
1. Download Qt from [qt.io](https://www.qt.io/download)
2. Install Qt6 or Qt5 with Qt Creator
3. Add Qt to your PATH

### Linux Installation

#### Ubuntu/Debian
```bash
# Install Qt6
sudo apt update
sudo apt install qt6-base-dev qt6-base-dev-tools

# Install CMake
sudo apt install cmake build-essential
```

#### CentOS/RHEL/Fedora
```bash
# Install Qt6
sudo dnf install qt6-qtbase-devel qt6-qtbase-devel-tools

# Install CMake
sudo dnf install cmake gcc-c++
```

## Building the GUI

### Method 1: Using the Build Script (Recommended)
```bash
# Make the build script executable
chmod +x build_gui.sh

# Run the build script
./build_gui.sh
```

The script will:
- Check for Qt installation
- Build the command-line version
- Build the GUI version (if Qt is available)
- Provide build status and executable locations

### Method 2: Using CMake
```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j$(nproc)

# The GUI executable will be: build/nanoex_gui
```

### Method 3: Manual Compilation
```bash
# For Qt6
g++ -std=c++17 -O2 -pthread \
    -I/opt/homebrew/include/qt6 \
    -I/opt/homebrew/include/qt6/QtCore \
    -I/opt/homebrew/include/qt6/QtWidgets \
    -I/opt/homebrew/include/qt6/QtGui \
    -fPIC \
    nanoex_gui.cpp \
    -o nanoex_gui \
    -L/opt/homebrew/lib \
    -lQt6Core -lQt6Widgets -lQt6Gui

# For Qt5 (adjust paths as needed)
g++ -std=c++17 -O2 -pthread \
    -I/usr/include/qt5 \
    -I/usr/include/qt5/QtCore \
    -I/usr/include/qt5/QtWidgets \
    -I/usr/include/qt5/QtGui \
    -fPIC \
    nanoex_gui.cpp \
    -o nanoex_gui \
    -lQt5Core -lQt5Widgets -lQt5Gui
```

## Running the GUI

### Starting the Application
```bash
# If built with build script
./nanoex_gui

# If built with CMake
./build/nanoex_gui

# Or double-click the executable in your file manager
```

### GUI Interface Overview

#### Main Window Layout
```
┌─────────────────────────────────────────────────────────────┐
│                    Menu Bar                                 │
├─────────────────────────────────────────────────────────────┤
│                    Control Panel                            │
│  [Start HFT System] [Clear Output]                         │
├─────────────────────────────────────────────────────────────┤
│ Performance Stats │ Strategy Performance                    │
│ Orders/sec: 25k   │ ┌─────────────────────────────────────┐ │
│ Latency: 50ns     │ │ Strategy │ Signals │ Orders │ PnL  │ │
│ Trades: 1,234     │ ├─────────────────────────────────────┤ │
│ Spread: $0.05     │ │ MeanRev  │   45    │   23   │ $12  │ │
│                   │ │ Momentum │   67    │   34   │ $8   │ │
│                   │ └─────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    System Output                            │
│ === NanoEX Performance Stats ===                           │
│ Runtime: 5000 ms                                           │
│ Orders processed: 125,000                                  │
│ ...                                                        │
└─────────────────────────────────────────────────────────────┘
│                    Status Bar                              │
└─────────────────────────────────────────────────────────────┘
```

#### Key Features

1. **Control Panel**
   - **Start/Stop Button**: Green when stopped, red when running
   - **Clear Output**: Clears the system output display

2. **Performance Widget**
   - Real-time key metrics display
   - Orders per second, latency, trades, spread
   - Auto-updates during system operation

3. **Strategy Performance Table**
   - Live updates of all active strategies
   - Signals generated, orders sent, PnL
   - Sortable columns

4. **System Output**
   - Real-time console output from HFT system
   - Auto-scrolling to latest output
   - Monospace font for readability

5. **Menu Bar**
   - **File → Compile**: Recompile the HFT system
   - **File → Save Log**: Save output to file
   - **Help → About**: Application information

## Usage Instructions

### Basic Operation
1. **Start the GUI**: Run `./nanoex_gui`
2. **Click "Start HFT System"**: The button will turn red and show "Stop HFT System"
3. **Monitor Performance**: Watch real-time metrics in the performance widget
4. **View Strategy Performance**: Check the strategy table for live updates
5. **Stop the System**: Click the red button to stop the HFT system

### Advanced Features
1. **Save Logs**: Use File → Save Log to save system output
2. **Recompile**: Use File → Compile to rebuild the HFT system
3. **Clear Output**: Use the Clear Output button to reset the display

### Keyboard Shortcuts
- **Ctrl+B**: Compile HFT System
- **Ctrl+S**: Save Log
- **Ctrl+Q**: Quit Application

## Troubleshooting

### Common Issues

#### 1. "Qt not found" Error
```bash
# Install Qt using Homebrew (macOS)
brew install qt6

# Or install Qt5
brew install qt5
```

#### 2. "CMake not found" Error
```bash
# Install CMake
brew install cmake  # macOS
sudo apt install cmake  # Ubuntu
```

#### 3. Compilation Errors
```bash
# Ensure C++17 support
g++ --version
# Should show version 7+ or Clang 5+

# Try building command-line version first
g++ -std=c++17 -O2 -pthread nanoex.cpp -o nanoex
```

#### 4. Runtime Errors
```bash
# Check if nanoex executable exists
ls -la nanoex

# Ensure it's executable
chmod +x nanoex

# Run from the same directory as nanoex
cd /path/to/HFT_System
./nanoex_gui
```

### Debug Mode
```bash
# Build with debug information
g++ -std=c++17 -g -O0 -pthread -Wall -Wextra nanoex_gui.cpp -o nanoex_gui_debug \
    -I/opt/homebrew/include/qt6 \
    -I/opt/homebrew/include/qt6/QtCore \
    -I/opt/homebrew/include/qt6/QtWidgets \
    -I/opt/homebrew/include/qt6/QtGui \
    -L/opt/homebrew/lib \
    -lQt6Core -lQt6Widgets -lQt6Gui

# Run debug version
./nanoex_gui_debug
```

## Performance Monitoring

### Real-time Metrics
The GUI displays these key metrics in real-time:
- **Orders/sec**: Processing throughput
- **Avg Latency**: Processing time per order (nanoseconds)
- **Trades**: Number of matched trades
- **Spread**: Current bid-ask spread

### Strategy Metrics
For each active strategy:
- **Signals Generated**: Trading signals produced
- **Orders Sent**: Orders submitted to matching engine
- **Orders Rejected**: Orders rejected by risk management
- **Current PnL**: Profit/loss for the strategy

## Customization

### Appearance
The GUI uses the system's default Qt theme. You can customize:
- Font sizes in the code
- Color schemes
- Window layout and sizing

### Configuration
Settings are automatically saved and restored:
- Window position and size
- Splitter positions
- Application preferences

## System Requirements

### Minimum Requirements
- **OS**: macOS 10.14+, Ubuntu 18.04+, or equivalent
- **RAM**: 4GB
- **Storage**: 100MB free space
- **CPU**: Multi-core processor recommended

### Recommended Requirements
- **OS**: macOS 12+, Ubuntu 20.04+
- **RAM**: 8GB+
- **Storage**: 500MB free space
- **CPU**: 4+ cores for optimal performance

## 🆘 Support

### Getting Help
1. Check the troubleshooting section above
2. Verify all prerequisites are installed
3. Try building the command-line version first
4. Check the system output for error messages

### Reporting Issues
When reporting issues, please include:
- Operating system and version
- Qt version (`qmake --version`)
- Compiler version (`g++ --version`)
- Error messages from the GUI
- Steps to reproduce the issue

---

**Note**: The GUI is designed to work alongside the command-line version. Both use the same underlying HFT system code, so performance characteristics are identical. 