#include <cmath>
#include "test_helpers.h"
#include "backtester.h"

// A scripted strategy: buys on bar `buy_at`, sells on bar `sell_at`.
static BacktestResult scripted(const std::vector<double>& prices, int buy_at, int sell_at, double qty = 10) {
    int bar = -1;
    auto fn = [&](const std::vector<std::shared_ptr<Order>>&) {
        ++bar;
        std::vector<std::shared_ptr<Order>> out;
        if (bar == buy_at || bar == sell_at) {
            auto o = make_tick(prices[bar]);
            o->side = bar == buy_at ? OrderSide::BUY : OrderSide::SELL;
            o->quantity = static_cast<Quantity>(qty);
            out.push_back(o);
        }
        return out;
    };
    Backtester bt;
    return bt.run(prices, "scripted", fn, [&]() { bar = -1; });
}

static void test_winning_trade() {
    auto r = scripted({100, 100, 110, 110}, 1, 2);
    CHECK(r.num_trades == 1);
    CHECK(r.winning_trades == 1);
    CHECK(std::abs(r.avg_trade_pct - 10.0) < 1e-9);
    CHECK(std::isinf(r.profit_factor));  // no losing trades
    CHECK(std::abs(r.total_return_pct - 0.1) < 1e-6);  // +$100 on $100k
}

static void test_losing_trade() {
    auto r = scripted({100, 100, 90, 90}, 1, 2);
    CHECK(r.num_trades == 1);
    CHECK(r.winning_trades == 0);
    CHECK(std::abs(r.avg_trade_pct + 10.0) < 1e-9);
    CHECK(r.profit_factor == 0.0);  // gross profit 0 / gross loss 100
    CHECK(r.total_return_pct < 0.0);
    CHECK(r.max_drawdown_pct > 0.0);
}

static void test_no_trades() {
    auto r = scripted({100, 101, 102}, 99, 99);
    CHECK(r.num_trades == 0);
    CHECK(r.profit_factor == 0.0 && r.avg_trade_pct == 0.0);
    CHECK(r.total_return_pct == 0.0);
    CHECK(r.equity_curve.size() == 3);
}

static void test_short_trade() {
    // Sell first (opens a short), then buy back lower.
    auto r = scripted({100, 100, 90, 90}, 2, 1);
    CHECK(r.num_trades == 1);
    CHECK(r.winning_trades == 1);
    CHECK(r.trades[0].is_buy == false);
}

static void test_sortino_sign() {
    auto up = scripted({100, 100, 110, 120, 115, 130}, 1, 5);
    CHECK(up.sortino_approx >= 0.0);
    auto down = scripted({100, 100, 90, 85, 80, 70}, 1, 5);
    CHECK(down.sortino_approx < 0.0);
}

int main() {
    test_winning_trade();
    test_losing_trade();
    test_no_trades();
    test_short_trade();
    test_sortino_sign();
    std::printf(g_failures ? "%d check(s) failed\n" : "all backtester tests passed\n", g_failures);
    return g_failures ? 1 : 0;
}
