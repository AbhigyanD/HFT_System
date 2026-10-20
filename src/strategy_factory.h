#pragma once
#include <memory>
#include <string>
#include "backtester.h"

// Order matters: the integer value is what the WASM API and web UI pass in.
enum class StrategyKind { Momentum, MeanReversion, Bollinger, Rsi, Breakout, Macd, Roc, Stochastic, EmaCross, ZScore };
constexpr StrategyKind kLastStrategyKind = StrategyKind::ZScore;

inline const char* strategy_name(StrategyKind k) {
    switch (k) {
        case StrategyKind::Momentum: return "Momentum";
        case StrategyKind::MeanReversion: return "Mean Reversion";
        case StrategyKind::Bollinger: return "Bollinger Bands";
        case StrategyKind::Rsi: return "RSI Reversal";
        case StrategyKind::Breakout: return "Breakout";
        case StrategyKind::Macd: return "MACD Crossover";
        case StrategyKind::Roc: return "Rate of Change";
        case StrategyKind::Stochastic: return "Stochastic";
        case StrategyKind::EmaCross: return "EMA Crossover";
        case StrategyKind::ZScore: return "Z-Score";
    }
    return "Unknown";
}

// Flat parameter set covering all strategies; percent values are in percent (2.0 = 2%).
struct StrategyParams {
    StrategyKind kind = StrategyKind::Momentum;
    double position_size = 50.0;
    double stop_loss_pct = 2.0;
    double take_profit_pct = 4.0;
    double trailing_stop_pct = 0.0;  // rule-based strategies only; 0 = off
    // Momentum
    size_t short_period = 5;
    size_t long_period = 20;
    double momentum_threshold = 0.25;
    // Mean reversion
    size_t lookback_period = 20;
    double entry_threshold_pct = 0.5;
    double exit_threshold_pct = 0.5;
    // Bollinger / RSI / Breakout / ROC use lookback_period as their window.
    double band_std = 2.0;          // Bollinger: band width in standard deviations
    double rsi_oversold = 30.0;     // RSI: buy below this
    double rsi_exit = 55.0;         // RSI: sell above this
    size_t macd_fast = 12, macd_slow = 26, macd_signal = 9;
    double roc_threshold_pct = 1.0; // ROC: buy when N-bar change exceeds this percent
    size_t ema_fast = 9, ema_slow = 21;  // EMA crossover periods
    double z_entry = 1.5, z_exit = 0.5;  // Z-score: buy below -z_entry, sell above z_exit (differs from Bollinger 2σ-in, mean-out)
    double stoch_oversold = 20.0;   // Stochastic: buy when %K falls below this
    double stoch_exit = 70.0;       // Stochastic: sell when %K rises above this
};

// Type-erased strategy so callers don't care which engine is behind it.
struct StrategyHandle {
    std::string name;
    Backtester::SignalFn generate;
    Backtester::ResetFn reset;
    std::function<std::string()> last_reason;  // reason for the most recent non-HOLD signal
};

StrategyHandle make_strategy(const StrategyParams& params);
