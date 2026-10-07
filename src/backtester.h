#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <string>
#include "order_book.h"

struct TradeRecord {
    size_t entry_bar;
    size_t exit_bar;
    double entry_price;
    double exit_price;
    bool is_buy;  // true = long, false = short
    double pnl_pct;
    double quantity;
};

struct BacktestResult {
    std::string strategy_name;
    double total_return_pct;
    size_t num_trades;
    size_t winning_trades;
    double win_rate_pct;
    double max_drawdown_pct;
    double sharpe_approx;  // annualized approx if bars are daily
    std::vector<TradeRecord> trades;
    std::vector<double> equity_curve;
};

class Backtester {
public:
    Backtester() = default;

    using SignalFn = std::function<std::vector<std::shared_ptr<Order>>(const std::vector<std::shared_ptr<Order>>&)>;
    using ResetFn = std::function<void()>;

    BacktestResult run(const std::vector<double>& prices,
                       const std::string& strategy_name,
                       SignalFn signal_fn,
                       ResetFn reset_fn);

private:
    static double compute_max_drawdown(const std::vector<double>& equity);
    static double compute_sharpe(const std::vector<double>& returns);
};
