#include "test_helpers.h"
#include "backtester.h"
#include "price_gen.h"
#include "strategy_factory.h"

static StrategyParams params(StrategyKind k, size_t lookback = 10) {
    StrategyParams p;
    p.kind = k;
    p.lookback_period = lookback;
    p.stop_loss_pct = 50.0;   // keep risk exits out of the way unless a test wants them
    p.take_profit_pct = 500.0;
    return p;
}

static std::vector<int> run(const StrategyParams& p, const std::vector<double>& prices) {
    StrategyHandle h = make_strategy(p);
    return run_series(h.generate, prices);
}

static int count(const std::vector<int>& v, int side) {
    int n = 0;
    for (int s : v) n += s == side;
    return n;
}

static void test_names() {
    CHECK(std::string(strategy_name(StrategyKind::Bollinger)) == "Bollinger Bands");
    StrategyParams p = params(StrategyKind::Macd);
    CHECK(make_strategy(p).name == "MACD Crossover");
}

static void test_no_signal_during_warmup() {
    for (StrategyKind k : {StrategyKind::Bollinger, StrategyKind::Rsi, StrategyKind::Breakout, StrategyKind::Roc}) {
        auto sides = run(params(k, 10), ramp(100, 90, 5));
        CHECK(count(sides, 0) == 5);
    }
}

static void test_bollinger() {
    // Quiet market, a sudden dip below the lower band, then recovery to the mean.
    auto prices = concat(concat(flat(100, 15), {99, 100, 99, 100, 99, 100}), concat({90}, ramp(90, 101, 12)));
    auto sides = run(params(StrategyKind::Bollinger, 10), prices);
    CHECK(sides[21] == 1);  // the dip bar is bought
    CHECK(count(sides, -1) >= 1);
    CHECK(sides[14] == 0);  // flat prices have zero band width: no signal
}

static void test_rsi() {
    auto prices = concat(flat(100, 3), concat(ramp(100, 80, 15), ramp(80, 110, 20)));
    auto sides = run(params(StrategyKind::Rsi, 10), prices);
    CHECK(count(sides, 1) >= 1);
    CHECK(count(sides, -1) >= 1);
    // never sells before it bought
    int pos = 0;
    for (int s : sides) { pos += s; CHECK(pos == 0 || pos == 1); }
}

static void test_breakout() {
    auto prices = concat(concat(flat(100, 12), {101}), concat(ramp(101, 110, 8), ramp(110, 95, 12)));
    auto sides = run(params(StrategyKind::Breakout, 10), prices);
    CHECK(sides[12] == 1);  // first close above the 10-bar high
    CHECK(count(sides, -1) == 1);
}

static void test_macd() {
    auto prices = concat(concat(flat(100, 40), ramp(100, 130, 30)), ramp(130, 90, 40));
    auto sides = run(params(StrategyKind::Macd), prices);
    CHECK(count(sides, 1) >= 1);
    CHECK(count(sides, -1) >= 1);
}

static void test_roc() {
    auto prices = concat(flat(100, 12), concat(ramp(100, 115, 10), ramp(115, 100, 10)));
    auto sides = run(params(StrategyKind::Roc, 5), prices);
    CHECK(count(sides, 1) == 1);
    CHECK(count(sides, -1) == 1);
}

static void test_stochastic() {
    // Range 100-120, then a drop to the bottom of the range (%K = 0), then a rally back up.
    auto prices = concat(concat(ramp(100, 120, 12), {100}), ramp(100, 119, 10));
    auto sides = run(params(StrategyKind::Stochastic, 10), prices);
    CHECK(sides[12] == 1);
    CHECK(count(sides, -1) >= 1);
    CHECK(count(run(params(StrategyKind::Stochastic, 10), flat(100, 30)), 1) == 0);  // no range, no signal
}

static void test_ema_cross() {
    StrategyParams p = params(StrategyKind::EmaCross);
    p.ema_fast = 3;
    p.ema_slow = 8;
    // Downtrend, then a sustained rally (up cross), then a sell-off (down cross).
    auto prices = concat(concat(ramp(110, 100, 15), ramp(100, 125, 15)), ramp(125, 95, 20));
    auto sides = run(p, prices);
    CHECK(count(sides, 1) == 1);
    CHECK(count(sides, -1) == 1);
    int buy = 0, sell = 0;
    for (size_t i = 0; i < sides.size(); ++i) { if (sides[i] == 1) buy = i; if (sides[i] == -1) sell = i; }
    CHECK(buy >= 15 && buy < 30);  // buys during the rally
    CHECK(sell > buy);
    CHECK(count(run(p, ramp(100, 130, 40)), -1) == 0);  // monotone rise never sells
}

static void test_ema_cross_reset() {
    StrategyParams p = params(StrategyKind::EmaCross);
    p.ema_fast = 3;
    p.ema_slow = 8;
    auto prices = concat(ramp(110, 100, 15), ramp(100, 125, 15));
    StrategyHandle h = make_strategy(p);
    auto first = run_series(h.generate, prices);
    h.reset();
    // Note: price history persists across reset, so only check that reset does not crash or sell while flat.
    auto again = run_series(h.generate, {125});
    CHECK(again[0] != -1);
    CHECK(count(first, 1) == 1);
}

static void test_zscore() {
    // Noisy-but-stable series, a sharp drop (large negative z), then a snap back above the mean.
    std::vector<double> base;
    for (int i = 0; i < 20; ++i) base.push_back(100 + (i % 2 ? 0.5 : -0.5));
    auto prices = concat(concat(base, {94}), {97, 100, 101});
    StrategyParams p = params(StrategyKind::ZScore, 20);
    auto sides = run(p, prices);
    CHECK(sides[20] == 1);
    CHECK(count(sides, -1) == 1);
    CHECK(count(run(p, flat(100, 40)), 1) == 0);  // zero variance: no signal
    p.z_entry = 50.0;  // unreachable threshold
    CHECK(count(run(p, prices), 1) == 0);
}

static void test_stop_loss_and_take_profit() {
    // Entry on a breakout, then a crash well below the stop.
    StrategyParams p = params(StrategyKind::Breakout, 5);
    p.stop_loss_pct = 2.0;
    auto sides = run(p, concat(concat(flat(100, 6), {101}), {99, 98, 97}));
    CHECK(sides[6] == 1);
    CHECK(sides[7] == -1);  // 99 is ~-2.0% from 101: stop fires immediately

    p = params(StrategyKind::Breakout, 5);
    p.take_profit_pct = 3.0;
    sides = run(p, concat(concat(flat(100, 6), {101}), {102, 104.5, 105}));
    CHECK(sides[6] == 1);
    CHECK(sides[9] == -1 || sides[8] == -1);
}

static void test_trailing_stop() {
    // Rally to 108 then fall: a 3% trailing stop must fire at ~104.7 even though the fixed stop (50%) never would.
    // Breakout(20) has no nearby exit (its 10-bar low is 100), so only the trailing stop can sell.
    StrategyParams p = params(StrategyKind::Breakout, 20);
    p.trailing_stop_pct = 3.0;
    auto prices = concat(concat(flat(100, 21), {101, 104, 108}), {107, 105, 104, 103});
    auto sides = run(p, prices);
    CHECK(sides[21] == 1);
    int sell = -1;
    for (size_t i = 0; i < sides.size(); ++i) if (sides[i] == -1) { sell = static_cast<int>(i); break; }
    CHECK(sell == 26);  // 104 <= 108 * 0.97 = 104.76
    // Disabled by default: same path, no trailing exit before the end.
    p.trailing_stop_pct = 0.0;
    sides = run(p, prices);
    CHECK(count(sides, -1) == 0);
}

static void test_reset_clears_position() {
    StrategyHandle h = make_strategy(params(StrategyKind::Breakout, 5));
    run_series(h.generate, concat(flat(100, 6), {101}));
    h.reset();
    // after reset the strategy is flat again, so it must not emit a sell
    auto sides = run_series(h.generate, {100});
    CHECK(sides[0] != -1);
}

static void test_backtests_run_and_are_deterministic() {
    auto prices = generate_prices(1500, 100.0, 0.0001, 0.01, 7);
    for (int k = 0; k <= static_cast<int>(kLastStrategyKind); ++k) {
        StrategyParams p = params(static_cast<StrategyKind>(k), 20);
        p.stop_loss_pct = 2.0;
        p.take_profit_pct = 4.0;
        StrategyHandle h1 = make_strategy(p), h2 = make_strategy(p);
        Backtester bt;
        auto r1 = bt.run(prices, h1.name, h1.generate, h1.reset);
        auto r2 = bt.run(prices, h2.name, h2.generate, h2.reset);
        CHECK(r1.equity_curve.size() == prices.size());
        CHECK(r1.total_return_pct == r2.total_return_pct);
        CHECK(r1.num_trades == r2.num_trades);
        CHECK(r1.win_rate_pct >= 0.0 && r1.win_rate_pct <= 100.0);
        CHECK(r1.max_drawdown_pct >= 0.0);
        CHECK(r1.profit_factor >= 0.0);
        CHECK((r1.num_trades == 0) == (r1.avg_trade_pct == 0.0 && r1.profit_factor == 0.0));
    }
}

int main() {
    test_names();
    test_no_signal_during_warmup();
    test_bollinger();
    test_rsi();
    test_breakout();
    test_macd();
    test_roc();
    test_stochastic();
    test_ema_cross();
    test_ema_cross_reset();
    test_zscore();
    test_stop_loss_and_take_profit();
    test_trailing_stop();
    test_reset_clears_position();
    test_backtests_run_and_are_deterministic();
    std::printf(g_failures ? "%d check(s) failed\n" : "all strategy tests passed\n", g_failures);
    return g_failures ? 1 : 0;
}
