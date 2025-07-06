# NanoEX High-Frequency Trading Engine

A high-performance, multi-threaded C++ trading engine designed for low-latency order matching and market data processing. This system demonstrates advanced concurrent programming techniques, race condition detection, and real-time financial data handling.

## 🏗️ Architecture Overview

### Core Components

1. **Matching Engine** - Order book management with price-time priority
2. **Market Data Feed** - Real-time price and trade data generation
3. **Strategy Engine** - Automated trading strategies (Mean Reversion, Momentum)
4. **Risk Management** - Position limits and loss controls
5. **Advanced Multi-threading** - Race condition detection and lock-free data structures

### System Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    Strategy Layer                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │Mean Reversion│  │  Momentum   │  │  Risk Mgmt  │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                   Market Data Layer                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ Data Feeds  │  │  Publisher  │  │  Consumers  │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                   Matching Engine Layer                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ Order Book  │  │  Trade Exec │  │  Order Mgmt │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│                Multi-threading Infrastructure               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │Lock-Free Q  │  │Work Stealing│  │Race Detection│        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 Features

### Step 1: Core Matching Engine
- **Order Types**: Limit and Market orders
- **Price-Time Priority**: FIFO order matching within price levels
- **Real-time Processing**: Nanosecond-level latency tracking
- **Performance Metrics**: Orders/sec, latency, spread analysis

### Step 2: Market Data Infrastructure
- **Multi-Symbol Feeds**: Concurrent price feeds for multiple instruments
- **Market Microstructure**: Realistic bid-ask spreads and volatility
- **High-Frequency Updates**: 1M+ updates/second capability
- **Event-Driven Architecture**: Publisher-subscriber pattern

### Step 3: Strategy Engine Framework
- **Mean Reversion Strategy**: Statistical arbitrage based on price deviations
- **Momentum Strategy**: Trend-following with RSI confirmation
- **Technical Indicators**: SMA, EMA, RSI, Momentum calculations
- **Risk Management**: Position limits, daily loss limits, order size controls

### Step 4: Advanced Multi-threading
- **Race Condition Detection**: Real-time monitoring of concurrent access
- **Lock-Free Data Structures**: High-performance queues without locks
- **Work-Stealing Thread Pool**: Dynamic load balancing across threads
- **Read-Write Locks**: Optimized concurrent access patterns
- **Barrier Synchronization**: Coordinated multi-threaded execution
- **Stress Testing**: Race condition simulation and validation

### GUI Application
- **Real-time System Control**: Start/stop HFT system with single click
- **Live Performance Monitoring**: Real-time display of orders/sec, latency, trades, spread
- **Strategy Performance Table**: Live updates of strategy metrics with sortable columns
- **System Output Display**: Real-time console output with auto-scrolling
- **Log Management**: Save and view system logs
- **Professional Interface**: Clean, modern Qt-based GUI with menu system
- **Settings Persistence**: Remembers window position and preferences
- **Cross-platform Support**: Works on macOS, Linux, and Windows

## 📋 Prerequisites

### Command-Line Version
- **Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **Platform**: Linux/macOS (tested on macOS 14.4 with Apple Clang 15.0.0)
- **Libraries**: Standard C++17 libraries only (no external dependencies)

### GUI Version
- **Qt Framework**: Qt5 or Qt6 (for GUI functionality)
- **CMake**: Version 3.16+ (optional, for easier building)
- **Platform**: macOS, Linux, Windows (cross-platform support)

## 🔧 Compilation

### Command-Line Version

#### Basic Compilation
```bash
g++ -std=c++17 -O2 -pthread nanoex.cpp -o nanoex
```

#### Optimized Compilation (Recommended)
```bash
g++ -std=c++17 -O3 -march=native -pthread -DNDEBUG nanoex.cpp -o nanoex
```

#### Debug Compilation
```bash
g++ -std=c++17 -g -O0 -pthread -Wall -Wextra nanoex.cpp -o nanoex
```

### GUI Version

#### Prerequisites
- **Qt Framework**: Qt5 or Qt6 (for GUI functionality)
- **CMake**: Version 3.16+ (optional, for easier building)

#### Installation of Dependencies

**macOS (using Homebrew):**
```bash
brew install qt6 cmake
```

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-base-dev-tools cmake build-essential
```

#### Building the GUI

**Method 1: Using the Build Script (Recommended)**
```bash
# Make the build script executable
chmod +x build_gui.sh

# Run the build script (builds both CLI and GUI)
./build_gui.sh
```

**Method 2: Using CMake**
```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j$(nproc)

# The GUI executable will be: build/nanoex_gui
```

**Method 3: Manual Compilation**
```bash
# For Qt6 on macOS
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
```

## 🎯 Usage

### Running the System

#### Command-Line Interface
```bash
./nanoex
```

#### Graphical User Interface (GUI)
```bash
# Build the GUI (if not already built)
./build_gui.sh

# Run the GUI application
open "build/NanoEX HFT System.app"  # macOS
# OR
./build/nanoex_gui  # Linux
```

### GUI Features
The GUI provides a professional interface with:
- **🟢 Real-time Control**: Start/stop HFT system with one click
- **📊 Live Performance Monitoring**: Orders/sec, latency, trades, spread
- **📈 Strategy Performance Table**: Live updates of all strategies
- **📝 System Output Display**: Real-time console output with auto-scroll
- **💾 Log Management**: Save and view system logs
- **🎨 Professional Interface**: Clean, modern Qt-based GUI

### Expected Output
The system will run a comprehensive demonstration including:

1. **Strategy Simulation** (5 seconds)
   - Background order generation: 25,000 orders/second
   - Market data feed: 300,000 updates/second
   - 3 active trading strategies

2. **Step 4 Demonstrations**
   - Race condition stress testing
   - Advanced matching engine performance
   - Lock-free queue operations
   - Read-write lock performance
   - Barrier synchronization

3. **Performance Statistics**
   - Orders processed per second
   - Average processing latency (nanoseconds)
   - Strategy performance metrics
   - Race condition detection results

## 📊 Performance Characteristics

### Latency Targets
- **Order Processing**: < 100 nanoseconds average
- **Market Data**: < 50 nanoseconds per update
- **Strategy Execution**: < 200 nanoseconds per signal

### Throughput Capabilities
- **Order Matching**: 100,000+ orders/second
- **Market Data**: 1,000,000+ updates/second
- **Strategy Processing**: 500,000+ signals/second

### Memory Usage
- **Order Book**: O(log n) per price level
- **Market Data**: Constant memory per symbol
- **Strategy State**: O(1) per strategy instance

## 🔍 Key Algorithms

### Order Matching Algorithm
```cpp
// Price-time priority matching
while (incoming_order.quantity > 0 && !opposite_side.empty()) {
    resting_order = opposite_side.get_best_order();
    if (can_match(incoming_order, resting_order)) {
        execute_trade(incoming_order, resting_order);
        update_order_quantities();
        if (resting_order.quantity == 0) {
            remove_from_order_book(resting_order);
        }
    }
}
```

### Race Condition Detection
```cpp
// Concurrent access monitoring
void log_access(const std::string& resource, const std::string& operation) {
    auto current_thread = std::this_thread::get_id();
    auto timestamp = std::chrono::steady_clock::now();
    
    // Check for potential race conditions
    if (operation == "write" && recent_access_by_different_thread(resource)) {
        potential_races_detected_++;
    }
}
```

## 🧪 Testing and Validation

### Built-in Tests
- **Race Condition Stress Test**: 8 threads, 5 seconds duration
- **Lock-Free Queue Test**: Producer-consumer pattern validation
- **Read-Write Lock Test**: Concurrent access pattern validation
- **Barrier Synchronization Test**: Multi-threaded coordination

### Performance Validation
- **Latency Measurement**: Nanosecond-precision timing
- **Throughput Analysis**: Orders/second processing rates
- **Memory Profiling**: Constant memory usage verification
- **Race Condition Monitoring**: Real-time detection and reporting

## 🔧 Configuration

### Market Data Parameters
```cpp
const double TICK_SIZE = 0.01;                    // $0.01 minimum increment
const double SPREAD_BASIS_POINTS = 5.0;          // 5 basis points spread
const double VOLATILITY_ANNUALIZED = 0.20;       // 20% annual volatility
```

### Strategy Parameters
```cpp
// Mean Reversion
double threshold = 0.015;                         // 1.5% deviation threshold
int lookback_period = 25;                         // 25-period SMA

// Momentum
double momentum_threshold = 0.008;                // 0.8% momentum threshold
double rsi_oversold = 25.0;                       // RSI oversold level
double rsi_overbought = 75.0;                     // RSI overbought level
```

### Risk Management
```cpp
double max_position_size = 10000.0;               // Max position value
double max_daily_loss = 5000.0;                   // Daily loss limit
double max_order_size = 1000.0;                   // Max order size
```

## 🐛 Troubleshooting

### Common Issues

#### Command-Line Version
1. **C++17 Extension Warnings**
   ```bash
   # Solution: Use -std=c++17 flag
   g++ -std=c++17 -O2 -pthread nanoex.cpp -o nanoex
   ```

2. **Threading Issues**
   ```bash
   # Ensure pthread support
   g++ -std=c++17 -pthread nanoex.cpp -o nanoex
   ```

3. **Performance Issues**
   ```bash
   # Use optimization flags
   g++ -std=c++17 -O3 -march=native -pthread nanoex.cpp -o nanoex
   ```

#### GUI Version
1. **"Qt not found" Error**
   ```bash
   # Install Qt using Homebrew (macOS)
   brew install qt6
   
   # Or install Qt5
   brew install qt5
   ```

2. **"CMake not found" Error**
   ```bash
   # Install CMake
   brew install cmake  # macOS
   sudo apt install cmake  # Ubuntu
   ```

3. **Compilation Errors**
   ```bash
   # Ensure C++17 support
   g++ --version
   # Should show version 7+ or Clang 5+
   
   # Try building command-line version first
   g++ -std=c++17 -O2 -pthread nanoex.cpp -o nanoex
   ```

4. **Runtime Errors**
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

#### Command-Line Version
```bash
# Compile with debug information
g++ -std=c++17 -g -O0 -pthread -Wall -Wextra nanoex.cpp -o nanoex

# Run with debugger
gdb ./nanoex
```

#### GUI Version
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

## 📈 Performance Monitoring

### Key Metrics
- **Orders Processed**: Total orders handled by matching engine
- **Trades Matched**: Successful order executions
- **Average Latency**: Processing time per order (nanoseconds)
- **Throughput**: Orders per second
- **Spread Analysis**: Bid-ask spread statistics
- **Race Conditions**: Detected concurrent access issues

### Real-time Monitoring

#### Command-Line Version
The system provides real-time performance statistics every second during execution, including:
- Matching engine performance
- Market data feed statistics
- Strategy performance metrics
- Multi-threading statistics

#### GUI Version
The GUI provides enhanced real-time monitoring with:
- **Live Performance Widget**: Real-time display of key metrics
- **Strategy Performance Table**: Live updates with sortable columns
- **System Output Display**: Real-time console output with auto-scrolling
- **Visual Indicators**: Color-coded performance status
- **Historical Tracking**: Performance trends over time

## 🔮 Future Enhancements

### Planned Features
1. **Network Layer**: TCP/UDP market data connectivity
2. **Database Integration**: Order persistence and analytics
3. **Web Interface**: Real-time monitoring dashboard
4. **Machine Learning**: AI-powered trading strategies
5. **Distributed Architecture**: Multi-node deployment
6. **Advanced GUI Features**: Charts, graphs, and advanced visualizations
7. **Configuration Management**: GUI-based system configuration
8. **Plugin Architecture**: Extensible strategy and indicator system

### Optimization Opportunities
1. **SIMD Instructions**: Vectorized order processing
2. **NUMA Awareness**: Multi-socket optimization
3. **GPU Acceleration**: CUDA/OpenCL integration
4. **Custom Allocators**: Memory pool optimization
5. **GUI Performance**: Hardware acceleration for real-time displays
6. **Memory Management**: Optimized GUI memory usage for high-frequency updates

## 📚 Technical Details

### Data Structures
- **Order Book**: Red-black tree for price levels, queues for orders
- **Market Data**: Lock-free queues for high-frequency updates
- **Strategy State**: Hash maps for position tracking
- **Thread Pool**: Work-stealing deque for load balancing
- **GUI Components**: Qt widgets for real-time display

### Synchronization Primitives
- **Mutex**: Standard library mutex for coarse-grained locking
- **Read-Write Locks**: Custom implementation for concurrent access
- **Atomic Operations**: Lock-free counters and flags
- **Barriers**: Synchronization points for multi-threaded coordination
- **Qt Threading**: QThread and QTimer for GUI responsiveness

### Memory Management
- **Smart Pointers**: RAII for automatic resource management
- **Move Semantics**: Efficient object transfer
- **Memory Pools**: Custom allocators for high-frequency objects
- **Qt Memory Management**: Automatic Qt object lifecycle management

### GUI Architecture
- **Qt Framework**: Cross-platform GUI framework
- **MVC Pattern**: Model-View-Controller for GUI organization
- **Event-Driven**: Signal-slot mechanism for real-time updates
- **Thread Safety**: Safe communication between HFT system and GUI

## 📄 License

This project is provided as educational software for demonstrating high-frequency trading concepts and advanced C++ programming techniques.

## 🤝 Contributing

This is a demonstration project showcasing advanced C++ programming concepts in the context of high-frequency trading systems. The code is designed to be educational and illustrative of real-world HFT system architecture.

## 📁 Project Structure

```
HFT_System/
├── nanoex.cpp              # Main HFT system (command-line)
├── nanoex_gui.cpp          # GUI application source
├── CMakeLists.txt          # CMake build configuration
├── build_gui.sh            # Automated build script
├── Info.plist.in           # macOS bundle configuration
├── GUI_INSTALL.md          # GUI installation guide
├── README.md               # This file
├── LICENSE                 # License file
└── build/                  # Build output directory
    ├── nanoex              # Command-line executable
    ├── nanoex_gui          # GUI executable
    └── NanoEX HFT System.app/  # macOS application bundle
```

## 🎯 Quick Start

1. **Build the system**: `./build_gui.sh`
2. **Run command-line**: `./nanoex`
3. **Run GUI**: `open "build/NanoEX HFT System.app"` (macOS) or `./build/nanoex_gui` (Linux)

---

**Note**: This system is for educational purposes only and should not be used for actual trading without proper regulatory compliance and risk management systems.