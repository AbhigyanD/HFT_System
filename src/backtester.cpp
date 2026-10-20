#include "backtester.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <numeric>

BacktestResult Backtester::run(const std::vector<double>& prices,
                              const std::string& strategy_name,
                              SignalFn signal_fn,
                              ResetFn reset_fn) {
    BacktestResult result;
    result.strategy_name = strategy_name;
    result.total_return_pct = 0;
    result.num_trades = 0;
    result.winning_trades = 0;
    result.win_rate_pct = 0;
    result.max_drawdown_pct = 0;
    result.sharpe_approx = 0;

    if (prices.size() < 2) return result;

    reset_fn();

    const double initial_capital = 100000.0;
    double position = 0;
    double entry_price = 0;
    size_t entry_bar = 0;
    double cash = initial_capital;
    std::vector<double> equity;
    equity.reserve(prices.size());

    OrderId next_id = 1;
    auto make_market_order = [&next_id](double price) {
        auto o = std::make_shared<Order>();
        o->order_id = next_id++;
        o->side = OrderSide::BUY;
        o->price = static_cast<Price>(price * 100.0);
        o->quantity = 1;
        o->type = OrderType::MARKET;
        o->timestamp = std::chrono::steady_clock::now();
        return o;
    };

    for (size_t i = 0; i < prices.size(); ++i) {
        double price = prices[i];
        std::vector<std::shared_ptr<Order>> bar_orders = { make_market_order(price) };
        auto signals = signal_fn(bar_orders);

        for (const auto& order : signals) {
            if (!order) continue;
            double qty = static_cast<double>(order->quantity);
            double fill_price = price;
            if (order->side == OrderSide::BUY) {
                if (position < 0) {
                    double close_qty = std::min(-position, qty);
                    cash -= fill_price * close_qty;
                    result.trades.push_back({ entry_bar, i, entry_price, fill_price, false,
                        (entry_price - fill_price) / entry_price * 100.0, close_qty });
                    result.num_trades++;
                    if (result.trades.back().pnl_pct > 0) result.winning_trades++;
                    position += close_qty;
                    if (position == 0) entry_price = 0;
                    qty -= close_qty;
                }
                if (qty > 0 && cash > 0) {
                    double max_qty = cash / fill_price;
                    qty = std::min(qty, max_qty);
                    if (qty > 0) {
                        position += qty;
                        entry_price = fill_price;
                        entry_bar = i;
                        cash -= fill_price * qty;
                    }
                }
            } else {
                if (position > 0) {
                    double close_qty = std::min(position, qty);
                    cash += fill_price * close_qty;
                    result.trades.push_back({ entry_bar, i, entry_price, fill_price, true,
                        (fill_price - entry_price) / entry_price * 100.0, close_qty });
                    result.num_trades++;
                    if (result.trades.back().pnl_pct > 0) result.winning_trades++;
                    position -= close_qty;
                    if (position == 0) entry_price = 0;
                    qty -= close_qty;
                }
                if (qty > 0 && position <= 0) {
                    double max_qty = (initial_capital + cash) / fill_price;
                    qty = std::min(qty, max_qty);
                    if (qty > 0) {
                        position -= qty;
                        entry_price = fill_price;
                        entry_bar = i;
                        cash += fill_price * qty;
                    }
                }
            }
        }

        double mark_value = position * price;
        equity.push_back(cash + mark_value);
    }

    result.equity_curve = std::move(equity);
    if (result.equity_curve.size() < 2) return result;

    double start_equity = result.equity_curve.front();
    double end_equity = result.equity_curve.back();
    if (start_equity != 0) {
        result.total_return_pct = (end_equity - start_equity) / std::abs(start_equity) * 100.0;
    } else {
        result.total_return_pct = (end_equity - initial_capital) / initial_capital * 100.0;
    }
    if (result.num_trades > 0) {
        result.win_rate_pct = 100.0 * static_cast<double>(result.winning_trades) / result.num_trades;
    }
    result.max_drawdown_pct = compute_max_drawdown(result.equity_curve);

    std::vector<double> returns;
    returns.reserve(result.equity_curve.size() - 1);
    for (size_t i = 1; i < result.equity_curve.size(); ++i) {
        if (result.equity_curve[i - 1] != 0) {
            returns.push_back((result.equity_curve[i] - result.equity_curve[i - 1]) / result.equity_curve[i - 1]);
        }
    }
    result.sharpe_approx = compute_sharpe(returns);
    result.sortino_approx = compute_sortino(returns);

    double gross_win = 0, gross_loss = 0, sum_pct = 0;
    for (const auto& t : result.trades) {
        double pnl = (t.exit_price - t.entry_price) * t.quantity * (t.is_buy ? 1.0 : -1.0);
        (pnl >= 0 ? gross_win : gross_loss) += std::abs(pnl);
        sum_pct += t.pnl_pct;
    }
    if (!result.trades.empty()) {
        result.avg_trade_pct = sum_pct / result.trades.size();
        result.profit_factor = gross_loss > 0 ? gross_win / gross_loss
                                              : (gross_win > 0 ? std::numeric_limits<double>::infinity() : 0.0);
    }
    return result;
}

double Backtester::compute_max_drawdown(const std::vector<double>& equity) {
    if (equity.empty()) return 0;
    double peak = equity[0];
    double max_dd = 0;
    for (double e : equity) {
        if (e > peak) peak = e;
        if (peak > 0) {
            double dd = (peak - e) / peak * 100.0;
            if (dd > max_dd) max_dd = dd;
        }
    }
    return max_dd;
}

double Backtester::compute_sharpe(const std::vector<double>& returns) {
    if (returns.size() < 2) return 0;
    double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    double mean = sum / returns.size();
    double sq_sum = 0;
    for (double r : returns) sq_sum += (r - mean) * (r - mean);
    double stddev = std::sqrt(sq_sum / (returns.size() - 1));
    if (stddev <= 0) return 0;
    return mean / stddev * std::sqrt(252);  // annualized assuming daily bars
}

double Backtester::compute_sortino(const std::vector<double>& returns) {
    if (returns.size() < 2) return 0;
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double downside = 0;
    for (double r : returns) if (r < 0) downside += r * r;
    double dd = std::sqrt(downside / returns.size());
    if (dd <= 0) return 0;
    return mean / dd * std::sqrt(252);
}
