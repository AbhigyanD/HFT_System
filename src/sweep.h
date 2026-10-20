#pragma once
#include <vector>
#include "strategy_factory.h"

// Result of running one parameter set over several synthetic price paths.
struct SweepRow {
    StrategyKind kind;
    size_t window;
    double avg_return_pct = 0;
    double avg_sharpe = 0;
    double avg_drawdown_pct = 0;
    double avg_trades = 0;
    double positive_paths_pct = 0;  // share of paths with a positive return
};

// Runs every (kind, window) pair over `seeds` random-walk paths of `bars` bars.
std::vector<SweepRow> run_sweep(const std::vector<StrategyKind>& kinds, const std::vector<size_t>& windows,
                                size_t bars, int seeds, double drift, double volatility,
                                const StrategyParams& base = StrategyParams());
