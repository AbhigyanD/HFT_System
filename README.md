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

## 📋 Prerequisites

- **Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **Platform**: Linux/macOS (tested on macOS 14.4 with Apple Clang 15.0.0)
- **Libraries**: Standard C++17 libraries only (no external dependencies)

## 🔧 Compilation

### Basic Compilation
```bash
g++ -std=c++17 -O2 -pthread nanoex.cpp -o nanoex
```

### Optimized Compilation (Recommended)
```bash
g++ -std=c++17 -O3 -march=native -pthread -DNDEBUG nanoex.cpp -o nanoex
```

### Debug Compilation
```bash
g++ -std=c++17 -g -O0 -pthread -Wall -Wextra nanoex.cpp -o nanoex
```

## 🎯 Usage

### Running the System
```bash
./nanoex
```

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

### Debug Mode
```bash
# Compile with debug information
g++ -std=c++17 -g -O0 -pthread -Wall -Wextra nanoex.cpp -o nanoex

# Run with debugger
gdb ./nanoex
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
The system provides real-time performance statistics every second during execution, including:
- Matching engine performance
- Market data feed statistics
- Strategy performance metrics
- Multi-threading statistics

## 🔮 Future Enhancements

### Planned Features
1. **Network Layer**: TCP/UDP market data connectivity
2. **Database Integration**: Order persistence and analytics
3. **Web Interface**: Real-time monitoring dashboard
4. **Machine Learning**: AI-powered trading strategies
5. **Distributed Architecture**: Multi-node deployment

### Optimization Opportunities
1. **SIMD Instructions**: Vectorized order processing
2. **NUMA Awareness**: Multi-socket optimization
3. **GPU Acceleration**: CUDA/OpenCL integration
4. **Custom Allocators**: Memory pool optimization

## 📚 Technical Details

### Data Structures
- **Order Book**: Red-black tree for price levels, queues for orders
- **Market Data**: Lock-free queues for high-frequency updates
- **Strategy State**: Hash maps for position tracking
- **Thread Pool**: Work-stealing deque for load balancing

### Synchronization Primitives
- **Mutex**: Standard library mutex for coarse-grained locking
- **Read-Write Locks**: Custom implementation for concurrent access
- **Atomic Operations**: Lock-free counters and flags
- **Barriers**: Synchronization points for multi-threaded coordination

### Memory Management
- **Smart Pointers**: RAII for automatic resource management
- **Move Semantics**: Efficient object transfer
- **Memory Pools**: Custom allocators for high-frequency objects

## 📄 License

This project is provided as educational software for demonstrating high-frequency trading concepts and advanced C++ programming techniques.

## 🤝 Contributing

This is a demonstration project showcasing advanced C++ programming concepts in the context of high-frequency trading systems. The code is designed to be educational and illustrative of real-world HFT system architecture.

---

**Note**: This system is for educational purposes only and should not be used for actual trading without proper regulatory compliance and risk management systems.