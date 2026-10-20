// Parameter sweep: average each strategy's results over many synthetic paths.
//   ./sweep [bars] [paths]
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "sweep.h"

int main(int argc, char** argv) {
    size_t bars = argc > 1 ? static_cast<size_t>(std::atoi(argv[1])) : 1500;
    int paths = argc > 2 ? std::atoi(argv[2]) : 20;
    if (bars < 100 || paths < 1) {
        std::fprintf(stderr, "usage: sweep [bars>=100] [paths>=1]\n");
        return 1;
    }

    std::vector<StrategyKind> kinds;
    for (int k = static_cast<int>(StrategyKind::MeanReversion); k <= static_cast<int>(kLastStrategyKind); ++k) {
        StrategyKind kind = static_cast<StrategyKind>(k);
        if (kind != StrategyKind::Macd && kind != StrategyKind::EmaCross) kinds.push_back(kind);  // fixed periods
    }
    auto rows = run_sweep(kinds, {10, 20, 40}, bars, paths, 0.0001, 0.01);
    std::sort(rows.begin(), rows.end(), [](const SweepRow& a, const SweepRow& b) { return a.avg_sharpe > b.avg_sharpe; });

    std::printf("%d paths x %zu bars (drift 0.01%%/bar, vol 1%%/bar), ranked by average Sharpe\n\n", paths, bars);
    std::printf("%-16s %6s %9s %8s %8s %8s %8s\n", "strategy", "window", "return%", "sharpe", "maxDD%", "trades", "win-paths%");
    for (const auto& r : rows)
        std::printf("%-16s %6zu %9.2f %8.2f %8.2f %8.1f %8.0f\n", strategy_name(r.kind), r.window, r.avg_return_pct,
                    r.avg_sharpe, r.avg_drawdown_pct, r.avg_trades, r.positive_paths_pct);
    std::printf("\nSynthetic random walks: a strategy that wins here is lucky, not skilled.\n");
    return 0;
}
