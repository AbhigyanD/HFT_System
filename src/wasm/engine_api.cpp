// C API for the browser demo. All results are returned as JSON strings that stay valid
// until the next call. Build with Emscripten (see CMakeLists.txt).
#include <emscripten/emscripten.h>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include "backtester.h"
#include "paper_sim.h"
#include "price_gen.h"
#include "strategy_factory.h"

namespace {

StrategyParams g_params;
PaperSim g_sim;
std::string g_out;

std::string num(double v) {
    if (!std::isfinite(v)) return "null";
    char buf[40];
    std::snprintf(buf, sizeof buf, "%.6g", v);
    return buf;
}

std::string quote(const std::string& s) {
    std::string o = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (static_cast<unsigned char>(c) < 0x20) o += ' ';
        else o += c;
    }
    return o + "\"";
}

const char* error(const char* msg) {
    g_out = std::string("{\"ok\":false,\"error\":") + quote(msg) + "}";
    return g_out.c_str();
}

const char* validate() {
    if (g_params.kind == StrategyKind::Momentum && g_params.short_period >= g_params.long_period)
        return "Short MA must be smaller than Long MA.";
    return nullptr;
}

}  // namespace

extern "C" {

// kind: StrategyKind value (0 momentum, 1 mean reversion, 2 bollinger, 3 rsi, 4 breakout, 5 macd, 6 roc). Percent values are in percent.
EMSCRIPTEN_KEEPALIVE void nx_set_params(int kind, double position_size, double stop_loss_pct, double take_profit_pct,
                                        int short_period, int long_period, double momentum_threshold,
                                        int lookback, double entry_pct, double exit_pct) {
    StrategyParams p;
    p.kind = static_cast<StrategyKind>(std::min(std::max(kind, 0), static_cast<int>(StrategyKind::Roc)));
    p.position_size = position_size;
    p.stop_loss_pct = stop_loss_pct;
    p.take_profit_pct = take_profit_pct;
    p.short_period = static_cast<size_t>(std::max(2, short_period));
    p.long_period = static_cast<size_t>(std::max(3, long_period));
    p.momentum_threshold = momentum_threshold;
    p.lookback_period = static_cast<size_t>(std::max(2, lookback));
    p.entry_threshold_pct = entry_pct;
    p.exit_threshold_pct = exit_pct;
    g_params = p;
}

EMSCRIPTEN_KEEPALIVE const char* nx_backtest(int bars, double drift_pct, double vol_pct, int seed) {
    if (const char* e = validate()) return error(e);
    bars = std::min(std::max(bars, 50), 20000);

    std::vector<double> prices = generate_prices(bars, 100.0, drift_pct / 100.0, vol_pct / 100.0,
                                                 static_cast<uint32_t>(seed));
    StrategyHandle h = make_strategy(g_params);
    Backtester bt;
    BacktestResult r = bt.run(prices, h.name, h.generate, h.reset);

    std::ostringstream o;
    o << "{\"ok\":true,\"name\":" << quote(r.strategy_name)
      << ",\"initial\":" << num(PaperSim::kInitialCash)
      << ",\"return_pct\":" << num(r.total_return_pct)
      << ",\"buy_hold_pct\":" << num((prices.back() / prices.front() - 1.0) * 100.0)
      << ",\"trades\":" << r.num_trades
      << ",\"win_rate\":" << num(r.win_rate_pct)
      << ",\"max_dd\":" << num(r.max_drawdown_pct)
      << ",\"sharpe\":" << num(r.sharpe_approx);

    o << ",\"prices\":[";
    for (size_t i = 0; i < prices.size(); ++i) o << (i ? "," : "") << num(prices[i]);
    o << "],\"equity\":[";
    for (size_t i = 0; i < r.equity_curve.size(); ++i) o << (i ? "," : "") << num(r.equity_curve[i]);

    // markers: [bar, 1 = buy / 0 = sell]; a long trade enters with a buy, a short with a sell.
    o << "],\"markers\":[";
    bool first = true;
    for (const auto& t : r.trades) {
        o << (first ? "" : ",") << "[" << t.entry_bar << "," << (t.is_buy ? 1 : 0) << "],["
          << t.exit_bar << "," << (t.is_buy ? 0 : 1) << "]";
        first = false;
    }
    o << "]}";
    g_out = o.str();
    return g_out.c_str();
}

EMSCRIPTEN_KEEPALIVE const char* nx_sim_start(double vol_pct, double start_price, int seed) {
    if (const char* e = validate()) return error(e);
    g_sim.start(g_params, vol_pct, start_price, static_cast<uint32_t>(seed));
    g_out = "{\"ok\":true}";
    return g_out.c_str();
}

EMSCRIPTEN_KEEPALIVE const char* nx_sim_step() {
    PaperSim::Step s = g_sim.step();
    std::ostringstream o;
    o << "{\"ok\":true,\"index\":" << s.index << ",\"price\":" << num(s.price)
      << ",\"equity\":" << num(s.equity) << ",\"realized\":" << num(s.realized)
      << ",\"position\":" << num(s.position) << ",\"trades\":" << s.trades << ",\"wins\":" << s.wins;
    if (s.traded) {
        o << ",\"trade\":{\"buy\":" << (s.buy ? "true" : "false") << ",\"qty\":" << num(s.qty)
          << ",\"pnl\":" << num(s.pnl_abs) << ",\"pnl_pct\":" << num(s.pnl_pct)
          << ",\"reason\":" << quote(s.reason) << "}";
    }
    o << "}";
    g_out = o.str();
    return g_out.c_str();
}

}  // extern "C"
