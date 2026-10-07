#include "strategy.h"
#include "mean_reversion_strategy.h"
#include "backtester.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <cmath>

std::vector<double> generate_prices(size_t n, double start = 100.0, double drift = 0.0001, double volatility = 0.02) {
    std::vector<double> prices;
    prices.reserve(n);
    std::mt19937 rng(42);
    std::normal_distribution<double> normal(0, 1);
    double s = start;
    for (size_t i = 0; i < n; ++i) {
        s = s * std::exp(drift + volatility * normal(rng));
        prices.push_back(s);
    }
    return prices;
}

void print_result(const BacktestResult& r) {
    std::cout << "\n--- " << r.strategy_name << " ---\n"
              << "  Total return: " << std::fixed << std::setprecision(2) << r.total_return_pct << "%\n"
              << "  Trades: " << r.num_trades << " (win rate " << r.win_rate_pct << "%)\n"
              << "  Max drawdown: " << r.max_drawdown_pct << "%\n"
              << "  Sharpe (ann.): " << std::setprecision(2) << r.sharpe_approx << "\n";
}

int main() {
    const size_t n_bars = 2000;
    std::cout << "Generating " << n_bars << " bars of synthetic price data...\n";
    std::vector<double> prices = generate_prices(n_bars);

    Backtester backtester;

    StrategyConfig momentum_config;
    momentum_config.momentum_threshold = 0.25;
    momentum_config.rsi_oversold = 25.0;
    momentum_config.rsi_overbought = 75.0;
    momentum_config.short_period = 5;
    momentum_config.long_period = 20;
    momentum_config.position_size = 50.0;
    momentum_config.stop_loss_pct = 1.5;
    momentum_config.take_profit_pct = 3.0;

    MeanReversionConfig mr_config;
    mr_config.lookback_period = 20;
    mr_config.entry_threshold_pct = 0.005;
    mr_config.exit_threshold_pct = 0.005;
    mr_config.position_size = 50.0;
    mr_config.stop_loss_pct = 2.0;
    mr_config.take_profit_pct = 4.0;

    StrategyEngine momentum(momentum_config);
    MeanReversionEngine mean_reversion(mr_config);

    std::cout << "Backtesting Momentum strategy...\n";
    auto res_momentum = backtester.run(
        prices,
        "Momentum",
        [&momentum](const std::vector<std::shared_ptr<Order>>& orders) {
            return momentum.generate_signals(orders);
        },
        [&momentum]() { momentum.reset_position(); }
    );
    print_result(res_momentum);

    std::cout << "Backtesting Mean Reversion strategy...\n";
    auto res_mr = backtester.run(
        prices,
        "Mean Reversion",
        [&mean_reversion](const std::vector<std::shared_ptr<Order>>& orders) {
            return mean_reversion.generate_signals(orders);
        },
        [&mean_reversion]() { mean_reversion.reset_position(); }
    );
    print_result(res_mr);

    std::cout << "\n--- Summary ---\n";
    if (res_momentum.total_return_pct > res_mr.total_return_pct) {
        std::cout << "Momentum had higher return on this path.\n";
    } else if (res_mr.total_return_pct > res_momentum.total_return_pct) {
        std::cout << "Mean Reversion had higher return on this path.\n";
    } else {
        std::cout << "Same return.\n";
    }
    std::cout << "Done.\n";
    return 0;
}
