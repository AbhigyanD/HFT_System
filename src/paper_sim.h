#pragma once
#include <cstdint>
#include <random>
#include <string>
#include "strategy_factory.h"

// Paper-trading simulation: random-walk market + one strategy + a long-only paper account.
// Single-threaded and UI-agnostic; the Qt app and the WebAssembly demo both drive it.
class PaperSim {
public:
    static constexpr double kInitialCash = 100000.0;

    struct Step {
        int index = 0;
        double price = 0;
        // Set when the strategy traded on this step.
        bool traded = false;
        bool buy = false;
        double qty = 0, pnl_abs = 0, pnl_pct = 0;
        std::string reason;
        // Account snapshot after the step.
        double equity = kInitialCash, cash = kInitialCash, position = 0, realized = 0;
        int trades = 0, wins = 0;
    };

    // volatility_pct is the per-tick standard deviation of log returns, in percent.
    void start(const StrategyParams& params, double volatility_pct, double start_price, uint32_t seed);
    Step step();
    Step snapshot() const;  // current state without advancing

private:
    StrategyHandle strategy_;
    std::mt19937 rng_;
    double volatility_ = 0.001;
    double price_ = 100.0;
    int index_ = 0;
    double cash_ = kInitialCash, position_ = 0, entry_ = 0, realized_ = 0;
    int trades_ = 0, wins_ = 0;
    uint64_t next_id_ = 1;
};
