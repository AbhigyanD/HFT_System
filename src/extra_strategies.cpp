#include "extra_strategies.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include "indicators.h"

namespace {
std::string fmt(const char* f, double a) {
    char buf[96];
    std::snprintf(buf, sizeof buf, f, a);
    return buf;
}
std::string fmt2(const char* f, double a, double b) {
    char buf[128];
    std::snprintf(buf, sizeof buf, f, a, b);
    return buf;
}
}  // namespace

void RuleStrategy::reset_position() {
    in_position_ = false;
    entry_price_ = 0.0;
    peak_price_ = 0.0;
    on_reset();
}

std::vector<std::shared_ptr<Order>> RuleStrategy::generate_signals(
    const std::vector<std::shared_ptr<Order>>& market_orders) {
    std::vector<std::shared_ptr<Order>> out;
    for (const auto& order : market_orders) {
        if (order->type != OrderType::MARKET) continue;
        prices_.push_back(static_cast<double>(order->price) / 100.0);
        if (prices_.size() > 2000) prices_.pop_front();
    }
    if (prices_.empty()) return out;

    on_bar(prices_);
    double price = prices_.back();
    bool buy = false;
    std::string reason;

    if (in_position_) {
        peak_price_ = std::max(peak_price_, price);
        double pnl = (price - entry_price_) / entry_price_ * 100.0;
        if (pnl <= -risk_.stop_loss_pct) reason = fmt("Stop loss (%.2f%%)", pnl);
        else if (pnl >= risk_.take_profit_pct) reason = fmt("Take profit (%.2f%%)", pnl);
        else if (risk_.trailing_stop_pct > 0.0 && price <= peak_price_ * (1.0 - risk_.trailing_stop_pct / 100.0))
            reason = fmt("Trailing stop (%.2f%% off peak)", (peak_price_ - price) / peak_price_ * 100.0);
        else if (!wants_exit(prices_, reason)) return out;
    } else {
        if (!wants_entry(prices_, reason)) return out;
        buy = true;
    }

    last_reason_ = reason;
    auto o = std::make_shared<Order>();
    o->order_id = static_cast<OrderId>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    o->side = buy ? OrderSide::BUY : OrderSide::SELL;
    o->price = static_cast<Price>(price * 100.0);
    o->quantity = static_cast<Quantity>(risk_.position_size);
    o->type = OrderType::MARKET;
    o->timestamp = std::chrono::steady_clock::now();
    out.push_back(o);

    in_position_ = buy;
    entry_price_ = buy ? price : 0.0;
    peak_price_ = buy ? price : 0.0;
    return out;
}

// ---- Bollinger bands ----
bool BollingerStrategy::bands(const std::deque<double>& p, double& mid, double& sd) const {
    if (period_ < 2 || p.size() < period_) return false;
    mid = Indicators::simple_moving_average(p, period_);
    double var = 0.0;
    for (size_t i = p.size() - period_; i < p.size(); ++i) var += (p[i] - mid) * (p[i] - mid);
    sd = std::sqrt(var / period_);
    return true;
}
bool BollingerStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    double mid, sd;
    if (!bands(p, mid, sd) || sd <= 0.0) return false;
    double lower = mid - k_ * sd;
    if (p.back() >= lower) return false;
    reason = fmt2("Below lower Bollinger band (%.2f < %.2f)", p.back(), lower);
    return true;
}
bool BollingerStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    double mid, sd;
    if (!bands(p, mid, sd) || p.back() < mid) return false;
    reason = fmt2("Back at middle band (%.2f >= %.2f)", p.back(), mid);
    return true;
}

// ---- RSI ----
bool RsiStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    if (p.size() < period_ + 1) return false;
    double rsi = Indicators::relative_strength_index(p, period_);
    if (rsi >= oversold_) return false;
    reason = fmt("RSI oversold (%.1f)", rsi);
    return true;
}
bool RsiStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    double rsi = Indicators::relative_strength_index(p, period_);
    if (rsi < exit_) return false;
    reason = fmt("RSI recovered (%.1f)", rsi);
    return true;
}

// ---- Donchian breakout ----
bool BreakoutStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    if (period_ < 2 || p.size() < period_ + 1) return false;
    double hi = *std::max_element(p.end() - 1 - period_, p.end() - 1);  // previous N bars
    if (p.back() <= hi) return false;
    reason = fmt2("Breakout above %d-bar high (%.2f)", static_cast<double>(period_), hi);
    return true;
}
bool BreakoutStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    size_t n = std::max<size_t>(2, period_ / 2);
    if (p.size() < n + 1) return false;
    double lo = *std::min_element(p.end() - 1 - n, p.end() - 1);
    if (p.back() >= lo) return false;
    reason = fmt2("Broke below %d-bar low (%.2f)", static_cast<double>(n), lo);
    return true;
}

// ---- MACD ----
bool MacdStrategy::cross(const std::deque<double>& p, double& hist, double& prev_hist) const {
    if (p.size() < slow_ + signal_ + 1) return false;
    auto [m, s] = Indicators::macd(p, fast_, slow_, signal_);
    std::deque<double> prev(p.begin(), p.end() - 1);
    auto [pm, ps] = Indicators::macd(prev, fast_, slow_, signal_);
    hist = m - s;
    prev_hist = pm - ps;
    return true;
}
bool MacdStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    double h, ph;
    if (!cross(p, h, ph) || !(ph <= 0.0 && h > 0.0)) return false;
    reason = fmt("MACD bullish crossover (hist %+.4f)", h);
    return true;
}
bool MacdStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    double h, ph;
    if (!cross(p, h, ph) || !(ph >= 0.0 && h < 0.0)) return false;
    reason = fmt("MACD bearish crossover (hist %+.4f)", h);
    return true;
}

// ---- Rate of change ----
bool RocStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    double roc = Indicators::price_change_percent(p, period_);
    if (p.size() < period_ + 1 || roc < threshold_) return false;
    reason = fmt("ROC momentum (%+.2f%%)", roc);
    return true;
}
bool RocStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    double roc = Indicators::price_change_percent(p, period_);
    if (roc >= 0.0) return false;
    reason = fmt("ROC turned negative (%+.2f%%)", roc);
    return true;
}

// ---- Stochastic oscillator ----
bool StochasticStrategy::percent_k(const std::deque<double>& p, double& k) const {
    if (period_ < 2 || p.size() < period_) return false;
    auto lo = std::min_element(p.end() - period_, p.end());
    auto hi = std::max_element(p.end() - period_, p.end());
    if (*hi <= *lo) return false;  // no range, %K undefined
    k = (p.back() - *lo) / (*hi - *lo) * 100.0;
    return true;
}
bool StochasticStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    double k;
    if (!percent_k(p, k) || k >= oversold_) return false;
    reason = fmt("Stochastic oversold (%%K %.1f)", k);
    return true;
}
bool StochasticStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    double k;
    if (!percent_k(p, k) || k < exit_) return false;
    reason = fmt("Stochastic recovered (%%K %.1f)", k);
    return true;
}

// ---- EMA crossover ----
void EmaCrossStrategy::on_bar(const std::deque<double>& p) {
    double price = p.back();
    if (!init_) {
        ema_fast_ = ema_slow_ = price;
        prev_diff_ = diff_ = 0.0;
        seen_ = 1;
        init_ = true;
        return;
    }
    double kf = 2.0 / (static_cast<double>(fast_) + 1.0), ks = 2.0 / (static_cast<double>(slow_) + 1.0);
    ema_fast_ = price * kf + ema_fast_ * (1.0 - kf);
    ema_slow_ = price * ks + ema_slow_ * (1.0 - ks);
    prev_diff_ = diff_;
    diff_ = ema_fast_ - ema_slow_;
    ++seen_;
}
bool EmaCrossStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    if (seen_ <= slow_ || !(prev_diff_ <= 0.0 && diff_ > 0.0)) return false;
    reason = fmt2("EMA%.0f crossed above EMA%.0f", static_cast<double>(fast_), static_cast<double>(slow_));
    return true;
}
bool EmaCrossStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    if (!(prev_diff_ >= 0.0 && diff_ < 0.0)) return false;
    reason = fmt2("EMA%.0f crossed below EMA%.0f", static_cast<double>(fast_), static_cast<double>(slow_));
    return true;
}

// ---- Z-score ----
bool ZScoreStrategy::zscore(const std::deque<double>& p, double& z) const {
    if (period_ < 2 || p.size() < period_) return false;
    double mean = Indicators::simple_moving_average(p, period_);
    double var = 0.0;
    for (size_t i = p.size() - period_; i < p.size(); ++i) var += (p[i] - mean) * (p[i] - mean);
    double sd = std::sqrt(var / period_);
    if (sd <= 1e-12) return false;
    z = (p.back() - mean) / sd;
    return true;
}
bool ZScoreStrategy::wants_entry(const std::deque<double>& p, std::string& reason) {
    double z;
    if (!zscore(p, z) || z > -entry_) return false;
    reason = fmt("Z-score stretched low (z %.2f)", z);
    return true;
}
bool ZScoreStrategy::wants_exit(const std::deque<double>& p, std::string& reason) {
    double z;
    if (!zscore(p, z) || z < exit_) return false;
    reason = fmt("Z-score reverted (z %.2f)", z);
    return true;
}

// ---- Ensemble ----
void EnsembleStrategy::on_bar(const std::deque<double>& p) {
    for (auto& m : members_) m->observe_bar(p);
}
void EnsembleStrategy::on_reset() {
    for (auto& m : members_) m->reset_position();
}
bool EnsembleStrategy::tally(const std::deque<double>& p, bool entry, std::string& reason) {
    size_t votes = 0;
    for (auto& m : members_) {
        std::string r;
        if (entry ? m->vote_entry(p, r) : m->vote_exit(p, r)) ++votes;
    }
    if (votes < min_votes_) return false;
    char buf[64];
    std::snprintf(buf, sizeof buf, "Ensemble %s (%zu/%zu votes)", entry ? "entry" : "exit", votes, members_.size());
    reason = buf;
    return true;
}
bool EnsembleStrategy::wants_entry(const std::deque<double>& p, std::string& reason) { return tally(p, true, reason); }
bool EnsembleStrategy::wants_exit(const std::deque<double>& p, std::string& reason) { return tally(p, false, reason); }
