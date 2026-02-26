# NanoEX High-Frequency Trading System

A high-performance, multi-threaded HFT system built in C++17 with advanced features including lock-free data structures, work-stealing thread pools, and comprehensive risk management.

---

## Visual GUI with Real-Time Charts

The GUI provides visual monitoring of the momentum strategy:

- **Strategy Chart Tab**: Real-time price line chart, buy/sell signal markers, and color-coded RSI, Momentum, MACD indicators
- **Mouse Hover Tooltips**: See price and time for any point on the chart
- **Live Signal Markers**: Green (BUY) and Red (SELL) dots on the price chart
- **Performance, Config, and Output Tabs**: All system metrics and logs

### Launching the GUI

From your project root, run:
```bash
./run_gui.sh
```
Or, from the build directory:
```bash
./NanoEX\ HFT\ System.app/Contents/MacOS/NanoEX\ HFT\ System
```

### GUI Tabs
- **Strategy Chart Tab**: Live price chart, buy/sell signals, and technical indicators
- **Performance Tab**: Orders processed, trades matched, latency, throughput
- **Strategy Config Tab**: All current strategy parameters
- **System Output Tab**: Full log and debug output

**Tip:** Hover your mouse over the chart to see price/time tooltips!

---

## Quick Start

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- Qt 5.12+ or Qt 6+ (for GUI)

### Building
```bash
mkdir build && cd build
cmake ..
make -j4
```

### Running
```bash
# Core system
./nanoex

# GUI (macOS)
open "NanoEX HFT System.app"
```

## Momentum Strategy

The system includes a **complete momentum strategy** that demonstrates the end-to-end workflow. This is perfect for new users to understand how the system works.

### Strategy Overview

The momentum strategy combines multiple technical indicators to identify trending markets and generate buy/sell signals:

- **Momentum Score**: Combines price vs moving averages and trend strength
- **RSI (Relative Strength Index)**: Identifies overbought/oversold conditions
- **MACD**: Confirms trend direction and momentum
- **Moving Averages**: Short-term (5 periods) vs long-term (20 periods) crossover signals

### Configuration

```cpp
StrategyConfig config;
config.momentum_threshold = 0.25;  // Minimum momentum to trigger signals
config.rsi_oversold = 25.0;        // RSI level for oversold condition
config.rsi_overbought = 75.0;      // RSI level for overbought condition
config.short_period = 5;            // Short-term MA period
config.long_period = 20;            // Long-term MA period
config.position_size = 50.0;       // Order size
config.stop_loss_pct = 1.5;        // Stop loss percentage
config.take_profit_pct = 3.0;      // Take profit percentage
```

### Signal Generation Logic

**BUY Conditions** (all must be true):
- Momentum score > threshold (0.25)
- RSI < overbought level (75)
- MACD line > signal line (bullish)
- Price > short-term moving average

**SELL Conditions** (any can trigger):
- Momentum score < 0 (weakening)
- RSI > overbought level (75)
- MACD line < signal line (bearish)
- Price < short-term moving average
- Stop loss or take profit hit

### Example Output

Example output:

```
BUY Signal: Momentum: 0.67, RSI: 57.14, MACD: Bullish, Price vs MA: Above (99.80 vs 99.65) (Confidence: 34.54%)
Order: BUY @ 100.00 x 50

SELL Signal: Momentum: -0.70, RSI: 50.00, MACD: Bearish, Price vs MA: Below (99.40 vs 99.55) (Confidence: 30.28%, P&L: -1.00%)
Order: SELL @ 99.00 x 50
```

### End-to-End Workflow

1. **Market Data Feed** → Generates simulated price data
2. **Strategy Engine** → Analyzes data with technical indicators
3. **Signal Generation** → Creates buy/sell signals based on momentum logic
4. **Risk Management** → Filters orders for compliance
5. **Matching Engine** → Executes orders and matches trades
6. **Performance Monitoring** → Tracks execution speed and P&L

### Learning the System

New users should:

1. **Run the system** and observe the real-time signals
2. **Study the configuration** in `src/main.cpp` (lines 25-35)
3. **Examine the strategy logic** in `src/strategy.cpp` (lines 95-130)
4. **Understand the indicators** in `src/indicators.cpp`
5. **Modify parameters** to see how they affect signal generation

### Customizing the Strategy

To create your own strategy:

1. **Modify `generate_momentum_signal()`** in `src/strategy.cpp`
2. **Add new indicators** in `src/indicators.h/cpp`
3. **Adjust configuration** in `src/main.cpp`
4. **Test different parameters** to optimize performance

## Architecture

### Core Components

- **Order Book**: Maintains bid/ask levels with lock-free operations
- **Matching Engine**: Executes orders with sub-microsecond latency
- **Market Data**: Simulates real-time price feeds
- **Strategy Engine**: Generates trading signals using technical analysis
- **Risk Manager**: Filters orders for compliance and risk limits
- **Performance Monitor**: Tracks system performance metrics
- **Thread Pool**: Work-stealing scheduler for optimal CPU utilization

### Advanced Features

- **Lock-free Data Structures**: Eliminates contention in hot paths
- **Work-stealing Thread Pool**: Dynamic load balancing across cores
- **Race Condition Detection**: Built-in debugging tools
- **Memory Pool**: Custom allocator for order objects
- **Real-time Monitoring**: Live performance metrics and system status

## Performance

- **Order Processing**: < 100 nanoseconds average
- **Throughput**: 1M+ orders/second on modern hardware
- **Memory Usage**: < 1MB for typical order book
- **Thread Scaling**: Linear performance up to CPU core count

## Configuration

### Build Options
```bash
# Debug build with race detection
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_RACE_DETECTION=ON ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_OPTIMIZATIONS=ON ..

# GUI build
cmake -DBUILD_GUI=ON ..
```

### Runtime Configuration
- Thread pool size: `std::thread::hardware_concurrency()`
- Order book depth: Configurable per instrument
- Risk limits: Adjustable per strategy
- Performance monitoring: Real-time metrics

## Troubleshooting

### Common Issues

**Build Errors:**
- Ensure C++17 compiler is installed
- Check Qt installation for GUI builds
- Verify CMake version (3.10+)

**Runtime Issues:**
- Monitor thread pool utilization
- Check memory usage patterns
- Verify order book integrity

**Performance Issues:**
- Profile with `perf` or `gprof`
- Monitor cache miss rates
- Check NUMA node affinity

### Debugging Tools

- **Race Detection**: Built-in thread safety validation
- **Performance Profiling**: Real-time metrics display
- **Order Book Visualization**: GUI shows live order book state
- **Signal Analysis**: Detailed strategy decision logging

## API Reference

### Core Classes

```cpp
// Order Book Management
class OrderBook {
    void add_order(std::shared_ptr<Order> order);
    std::pair<Price, Price> get_best_bid_ask() const;
    size_t get_processed_orders() const;
};

// Strategy Engine
class StrategyEngine {
    std::vector<std::shared_ptr<Order>> generate_signals(const std::vector<std::shared_ptr<Order>>& market_orders);
    void set_config(const StrategyConfig& config);
};

// Risk Management (configurable limits: max order size, notional, orders per batch, daily volume)
class RiskManager {
    void set_config(const RiskConfig& config);
    std::vector<std::shared_ptr<Order>> filter_orders(const std::vector<std::shared_ptr<Order>>& orders);
    uint64_t get_orders_rejected() const;
};
```

### Technical Indicators

```cpp
class Indicators {
    static double simple_moving_average(const std::deque<double>& values, size_t period);
    static double relative_strength_index(const std::deque<double>& prices, size_t period);
    static std::pair<double, double> macd(const std::deque<double>& prices, size_t fast, size_t slow, size_t signal);
    static double momentum_score(const std::deque<double>& prices, size_t short_period, size_t long_period);
};
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by modern HFT systems and academic research
- Built with performance and reliability in mind
- Designed for educational and research purposes

---

**Note**: This system is for educational and research purposes. Real trading systems require additional safety measures, compliance features, and extensive testing.