#pragma once
#include <memory>
#include <string>
#include "backtester.h"

enum class StrategyKind { Momentum, MeanReversion };

// Flat parameter set covering both strategies; percent values are in percent (2.0 = 2%).
struct StrategyParams {
    StrategyKind kind = StrategyKind::Momentum;
    double position_size = 50.0;
    double stop_loss_pct = 2.0;
    double take_profit_pct = 4.0;
    // Momentum
    size_t short_period = 5;
    size_t long_period = 20;
    double momentum_threshold = 0.25;
    // Mean reversion
    size_t lookback_period = 20;
    double entry_threshold_pct = 0.5;
    double exit_threshold_pct = 0.5;
};

// Type-erased strategy so callers don't care which engine is behind it.
struct StrategyHandle {
    std::string name;
    Backtester::SignalFn generate;
    Backtester::ResetFn reset;
    std::function<std::string()> last_reason;  // reason for the most recent non-HOLD signal
};

StrategyHandle make_strategy(const StrategyParams& params);
