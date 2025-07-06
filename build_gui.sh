#!/bin/bash

# NanoEX HFT System GUI Build Script
# This script builds both the command-line and GUI versions of the HFT system

set -e  # Exit on any error

echo "=== NanoEX HFT System Build Script ==="
echo ""

# Check if Qt is available
QT_AVAILABLE=false
if command -v qmake &> /dev/null; then
    QT_VERSION=$(qmake -query QT_VERSION 2>/dev/null || echo "unknown")
    echo "Found Qt version: $QT_VERSION"
    QT_AVAILABLE=true
elif command -v qt6-config &> /dev/null; then
    echo "Found Qt6"
    QT_AVAILABLE=true
elif command -v qt5-config &> /dev/null; then
    echo "Found Qt5"
    QT_AVAILABLE=true
else
    echo "Warning: Qt not found. GUI will not be built."
    echo "Install Qt5 or Qt6 to build the GUI version."
    echo ""
fi

# Build command-line version
echo "Building command-line version..."
g++ -std=c++17 -O2 -pthread nanoex.cpp -o nanoex
if [ $? -eq 0 ]; then
    echo "✓ Command-line version built successfully"
else
    echo "✗ Failed to build command-line version"
    exit 1
fi

# Build GUI version if Qt is available
if [ "$QT_AVAILABLE" = true ]; then
    echo ""
    echo "Building GUI version..."
    
    # Try to use CMake if available
    if command -v cmake &> /dev/null; then
        echo "Using CMake build system..."
        
        # Create build directory
        mkdir -p build
        cd build
        
        # Configure and build
        cmake ..
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
        
        if [ $? -eq 0 ]; then
            echo "✓ GUI version built successfully with CMake"
            echo "GUI executable: build/nanoex_gui"
        else
            echo "✗ Failed to build GUI version with CMake"
            cd ..
        fi
    else
        echo "CMake not found, trying direct compilation..."
        
        # Try to find Qt installation
        QT_INCLUDE=""
        QT_LIBS=""
        
        # Common Qt installation paths
        QT_PATHS=(
            "/usr/include/qt6"
            "/usr/include/qt5"
            "/opt/homebrew/include/qt6"
            "/opt/homebrew/include/qt5"
            "/usr/local/include/qt6"
            "/usr/local/include/qt5"
        )
        
        for path in "${QT_PATHS[@]}"; do
            if [ -d "$path" ]; then
                QT_INCLUDE="$path"
                break
            fi
        done
        
        if [ -n "$QT_INCLUDE" ]; then
            echo "Found Qt headers at: $QT_INCLUDE"
            
            # Try to compile directly
            g++ -std=c++17 -O2 -pthread \
                -I"$QT_INCLUDE" \
                -I"$QT_INCLUDE/QtCore" \
                -I"$QT_INCLUDE/QtWidgets" \
                -I"$QT_INCLUDE/QtGui" \
                -fPIC \
                nanoex_gui.cpp \
                -o nanoex_gui \
                -lQt6Core -lQt6Widgets -lQt6Gui \
                -lQt5Core -lQt5Widgets -lQt5Gui \
                2>/dev/null || echo "Direct compilation failed"
            
            if [ -f "nanoex_gui" ]; then
                echo "✓ GUI version built successfully with direct compilation"
            else
                echo "✗ Failed to build GUI version"
            fi
        else
            echo "✗ Could not find Qt headers"
        fi
    fi
else
    echo ""
    echo "Skipping GUI build (Qt not available)"
fi

echo ""
echo "=== Build Summary ==="
if [ -f "nanoex" ]; then
    echo "✓ Command-line version: ./nanoex"
fi

if [ -f "nanoex_gui" ]; then
    echo "✓ GUI version: ./nanoex_gui"
elif [ -f "build/nanoex_gui" ]; then
    echo "✓ GUI version: ./build/nanoex_gui"
else
    echo "✗ GUI version: Not built (Qt required)"
fi

echo ""
echo "To run the command-line version:"
echo "  ./nanoex"
echo ""
echo "To run the GUI version (if built):"
echo "  ./nanoex_gui"
echo ""

# Make executables executable
chmod +x nanoex 2>/dev/null || true
chmod +x nanoex_gui 2>/dev/null || true
chmod +x build/nanoex_gui 2>/dev/null || true 