#include <cmath>
#include "test_helpers.h"
#include "sweep.h"

static void test_shape() {
    auto rows = run_sweep({StrategyKind::Bollinger, StrategyKind::Rsi}, {10, 20}, 400, 3, 0.0001, 0.01);
    CHECK(rows.size() == 4);
    CHECK(rows[0].kind == StrategyKind::Bollinger && rows[0].window == 10);
    CHECK(rows[3].kind == StrategyKind::Rsi && rows[3].window == 20);
    for (const auto& r : rows) {
        CHECK(std::isfinite(r.avg_return_pct) && std::isfinite(r.avg_sharpe));
        CHECK(r.positive_paths_pct >= 0.0 && r.positive_paths_pct <= 100.0);
        CHECK(r.avg_drawdown_pct >= 0.0);
        CHECK(r.avg_trades >= 0.0);
    }
}

static void test_deterministic() {
    auto a = run_sweep({StrategyKind::Roc}, {10}, 500, 4, 0.0, 0.01);
    auto b = run_sweep({StrategyKind::Roc}, {10}, 500, 4, 0.0, 0.01);
    CHECK(a.size() == 1 && b.size() == 1);
    CHECK(a[0].avg_return_pct == b[0].avg_return_pct);
    CHECK(a[0].avg_trades == b[0].avg_trades);
}

static void test_edge_cases() {
    CHECK(run_sweep({StrategyKind::Rsi}, {10}, 300, 0, 0.0, 0.01).empty());   // no paths
    CHECK(run_sweep({}, {10}, 300, 2, 0.0, 0.01).empty());                     // no strategies
    CHECK(run_sweep({StrategyKind::Rsi}, {}, 300, 2, 0.0, 0.01).empty());      // no windows
}

static void test_zero_vol_means_no_trading() {
    // Constant prices: nothing to trade, so no return and no trades for range-based strategies.
    auto rows = run_sweep({StrategyKind::Stochastic, StrategyKind::ZScore, StrategyKind::Bollinger}, {10}, 300, 2, 0.0, 0.0);
    for (const auto& r : rows) {
        CHECK(r.avg_trades == 0.0);
        CHECK(r.avg_return_pct == 0.0);
    }
}

int main() {
    test_shape();
    test_deterministic();
    test_edge_cases();
    test_zero_vol_means_no_trading();
    std::printf(g_failures ? "%d check(s) failed\n" : "all sweep tests passed\n", g_failures);
    return g_failures ? 1 : 0;
}
