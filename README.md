# NanoEX

A C++17 trading-engine sandbox: a limit-order-book matching engine, a set of technical trading strategies, a backtester, and a desktop app for running strategies live against a simulated market.

> **Educational project.** All market data is synthetic (random walks). Results show how strategies behave mechanically, not how they would perform on real markets, and nothing here is trading advice.

## Live demo

The engine also compiles to WebAssembly: [`web/`](web/) is a static page that runs the same C++ strategies and backtester in the browser, with no server. It deploys to GitHub Pages from `.github/workflows/pages.yml` (repo Settings → Pages → Source: **GitHub Actions**).

To run it locally you need the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
emcmake cmake -S . -B build-wasm && cmake --build build-wasm
cp build-wasm/engine.js web/engine.js
python3 -m http.server -d web 8000     # then open http://localhost:8000
```

## Features

- **Desktop app** (Qt 6): live simulation with a price chart and trade markers, a backtester with an equity curve, and editable strategy and risk settings. No coding needed.
- **Eleven strategies**: Momentum, Mean Reversion, Bollinger Bands, RSI Reversal, Breakout, MACD Crossover, Rate of Change, Stochastic, EMA Crossover, Z-Score and an Ensemble Vote, all with stop loss and take profit.
- **Backtester**: runs a strategy over a price series and reports return, trades, win rate, max drawdown and an annualised Sharpe ratio.
- **Matching engine**: price-time priority limit and market orders, partial fills, cancels, mutex-protected.
- **Risk manager**: per-order quantity and notional caps, a per-batch order limit and a daily volume cap.
- **Command-line tools**: a headless live simulation and a headless backtest.

## Quick start

**Requirements:** a C++17 compiler, CMake 3.16+, and Qt 6 for the desktop app (`brew install qt` on macOS). Without Qt, the command-line tools still build.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"   # prefix hint only if Qt isn't found automatically
cmake --build build -j
```

| Target | Run | What it does |
| --- | --- | --- |
| Desktop app | `open build/NanoEX.app` (macOS) or `./build/NanoEX` | Live simulation and backtesting UI |
| `backtest` | `./build/backtest` | Backtests every strategy on 2000 synthetic bars and prints a summary |
| `nanoex` | `./build/nanoex` | Headless live simulation; press Enter to stop |

## Using the desktop app

1. **Pick a strategy** in the sidebar and adjust its parameters, position size, stop loss and take profit.
2. **Live Simulation tab**: press **Start**. A simulated market ticks at the interval you choose. The chart marks buys (▲) and sells (▼), the cards show equity, realized P&L, position and win rate, and every trade appears in the log with the reason for the signal. **Export trades to CSV** saves the log.
3. **Backtest tab**: set the length, volatility, drift and seed of the synthetic price series, then press **Run backtest**. Compare the strategy's return with buy & hold, and read drawdown and Sharpe off the cards. Using the same seed reproduces the same price path.

Sidebar changes apply the next time you press Start or Run backtest.

The live tab runs a simple long-only paper account with $100,000 of starting cash. The backtester also supports short positions.

## Strategies

All strategies are long-only, share the same stop loss and take profit exits, and use a window (`Window` in the apps) where noted. Defaults are in `StrategyParams` in [src/strategy_factory.h](src/strategy_factory.h).

| Strategy | Buys when | Sells when |
| --- | --- | --- |
| Bollinger Bands | price closes below the lower band (2 std devs) | price returns to the middle band |
| RSI Reversal | RSI < 30 | RSI >= 55 |
| Breakout | price exceeds the previous N-bar high | price breaks the N/2-bar low |
| MACD Crossover | MACD crosses above its signal line | MACD crosses below it |
| Rate of Change | N-bar change >= 1% | N-bar change turns negative |
| Stochastic | %K < 20 | %K >= 70 |
| EMA Crossover | EMA 9 crosses above EMA 21 | EMA 9 crosses below EMA 21 |
| Z-Score | z < -1.5 versus the rolling mean | z >= 0.5 |
| Ensemble Vote | at least 2 of Bollinger, RSI, Stochastic, Z-Score agree to buy | at least 2 agree to sell |

## Tests

```bash
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure
```


### Momentum

Buys when all of these hold:
- the momentum score is above the threshold (the score combines price vs the short SMA, short SMA vs long SMA, and recent price change)
- RSI is below the overbought level
- the MACD line is above its signal line
- price is above the short SMA

Sells when **any** of these hold: momentum turns negative, RSI is overbought, MACD turns bearish, price falls below the short SMA, or the stop loss or take profit is hit.

| Parameter | Default (app) |
| --- | --- |
| Short / long SMA | 5 / 20 bars |
| Momentum threshold | 0.25 |
| Stop loss / take profit | 2% / 4% |

RSI uses a simple average of the last 14 changes. MACD uses 12 / 26 / 9 EMAs.

### Mean Reversion

Buys when price falls below its SMA by more than the entry threshold. Sells when price rises above the SMA by more than the exit threshold, or on stop loss or take profit.

| Parameter | Default (app) |
| --- | --- |
| SMA lookback | 20 bars |
| Buy below SMA | 0.5% |
| Sell above SMA | 0.5% |
| Stop loss / take profit | 2% / 4% |

On the default 2000-bar synthetic path (seed 42) the CLI `backtest` currently reports about +10.8% for mean reversion and about −7.6% for momentum. This is a single random path, so don't read anything into the ranking.

## Project layout

```
src/
  order_book.*, matching_engine.*   limit order book and matching
  indicators.*                      SMA, RSI, MACD, momentum score
  strategy.*                        momentum strategy
  mean_reversion_strategy.*         mean-reversion strategy
  strategy_factory.*                builds any strategy from one parameter struct
  extra_strategies.*                rule-based strategies (Bollinger, RSI, breakout, MACD, ROC, stochastic, EMA cross, Z-score)
tests/                              strategy test suite (CTest)
  backtester.*, price_gen.h         backtest loop and synthetic price generator
  risk.*, performance.*             risk limits, throughput counter
  market_data.*, threading.*        simulated order feed, thread pool
  main.cpp, backtest_main.cpp       command-line entry points
  paper_sim.*                       live paper-trading simulation shared by the app and the web demo
  gui/                              Qt desktop app (window, chart widget, simulation worker)
  wasm/                             C API exported to JavaScript for the web demo
web/                                browser UI (HTML/CSS/JS) for the WebAssembly build
examples/                           standalone strategy example
```

The engine builds as a static library, `nanoex_core`, with no Qt dependency. The desktop app and both command-line tools link against it.

## Extending it

**Add a strategy.** Implement an engine with a `generate_signals(market_orders)` method that returns orders, as `StrategyEngine` and `MeanReversionEngine` do. Then add it to `make_strategy()` in [src/strategy_factory.cpp](src/strategy_factory.cpp). The backtester and the app both consume the factory's `StrategyHandle`.

**Add an indicator.** Add a static method to `Indicators` in [src/indicators.h](src/indicators.h) and [src/indicators.cpp](src/indicators.cpp).

**Change the live CLI setup.** Strategy parameters are set in [src/main.cpp](src/main.cpp), and risk limits come from `RiskConfig` in [src/risk.h](src/risk.h) (all limits are off by default).

See [MOMENTUM_STRATEGY_GUIDE.md](MOMENTUM_STRATEGY_GUIDE.md) for a longer walkthrough of the momentum strategy and [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

## Limitations

- Market data is synthetic. There is no real-data feed or CSV loader yet.
- The backtester fills every order at the bar's price: no slippage, spread, fees or latency.
- The live tab simulates fills with a paper account. It does not route strategy orders through the matching engine. The `nanoex` CLI does.
- The thread pool is a plain FIFO pool, and the matching engine serialises orders behind one mutex. This is not a low-latency design.
- There are no automated tests yet.

## License

MIT. See [LICENSE](LICENSE).
