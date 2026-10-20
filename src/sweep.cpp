#include "sweep.h"
#include "backtester.h"
#include "price_gen.h"

std::vector<SweepRow> run_sweep(const std::vector<StrategyKind>& kinds, const std::vector<size_t>& windows,
                                size_t bars, int seeds, double drift, double volatility,
                                const StrategyParams& base) {
    std::vector<SweepRow> rows;
    if (seeds <= 0) return rows;

    std::vector<std::vector<double>> paths;
    for (int s = 0; s < seeds; ++s)
        paths.push_back(generate_prices(bars, 100.0, drift, volatility, static_cast<uint32_t>(s + 1)));

    for (StrategyKind kind : kinds) {
        for (size_t window : windows) {
            StrategyParams p = base;
            p.kind = kind;
            p.lookback_period = window;
            SweepRow row;
            row.kind = kind;
            row.window = window;
            int positive = 0;
            for (const auto& path : paths) {
                StrategyHandle h = make_strategy(p);
                Backtester bt;
                BacktestResult r = bt.run(path, h.name, h.generate, h.reset);
                row.avg_return_pct += r.total_return_pct;
                row.avg_sharpe += r.sharpe_approx;
                row.avg_drawdown_pct += r.max_drawdown_pct;
                row.avg_trades += static_cast<double>(r.num_trades);
                positive += r.total_return_pct > 0 ? 1 : 0;
            }
            double n = static_cast<double>(paths.size());
            row.avg_return_pct /= n;
            row.avg_sharpe /= n;
            row.avg_drawdown_pct /= n;
            row.avg_trades /= n;
            row.positive_paths_pct = 100.0 * positive / n;
            rows.push_back(row);
        }
    }
    return rows;
}
