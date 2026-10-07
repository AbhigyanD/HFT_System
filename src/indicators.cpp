#include "indicators.h"
#include <numeric>
#include <algorithm>

// Simple Moving Average
double Indicators::simple_moving_average(const std::deque<double>& values, size_t period) {
    if (values.size() < period || period == 0) return 0.0;
    double sum = std::accumulate(values.end() - period, values.end(), 0.0);
    return sum / period;
}

// Relative Strength Index (simple average of the last `period` changes)
double Indicators::relative_strength_index(const std::deque<double>& prices, size_t period) {
    if (period == 0 || prices.size() < period + 1) return 50.0; // Neutral RSI

    double gain_sum = 0.0, loss_sum = 0.0;
    for (size_t i = prices.size() - period; i < prices.size(); ++i) {
        double change = prices[i] - prices[i - 1];
        if (change > 0) gain_sum += change;
        else loss_sum -= change;
    }
    if (loss_sum == 0.0) return 100.0;

    double rs = (gain_sum / period) / (loss_sum / period);
    return 100.0 - (100.0 / (1.0 + rs));
}

namespace {
// EMA over the whole series, seeded with the first value.
std::vector<double> ema_series(const std::vector<double>& values, size_t period) {
    std::vector<double> out;
    out.reserve(values.size());
    double k = 2.0 / (static_cast<double>(period) + 1.0);
    for (size_t i = 0; i < values.size(); ++i) {
        out.push_back(i == 0 ? values[0] : values[i] * k + out.back() * (1.0 - k));
    }
    return out;
}
}  // namespace

// MACD: (EMA(fast) - EMA(slow), EMA(signal) of that line). O(n).
std::pair<double, double> Indicators::macd(const std::deque<double>& prices, size_t fast_period, size_t slow_period, size_t signal_period) {
    if (prices.size() < slow_period || fast_period == 0 || slow_period == 0 || signal_period == 0) return {0.0, 0.0};

    std::vector<double> series(prices.begin(), prices.end());
    std::vector<double> fast = ema_series(series, fast_period);
    std::vector<double> slow = ema_series(series, slow_period);
    std::vector<double> line(series.size());
    for (size_t i = 0; i < series.size(); ++i) line[i] = fast[i] - slow[i];

    std::vector<double> signal = ema_series(line, signal_period);
    return {line.back(), signal.back()};
}

// Price Change Percentage
double Indicators::price_change_percent(const std::deque<double>& prices, size_t period) {
    if (prices.size() < period + 1) return 0.0;
    
    double current_price = prices.back();
    double past_price = prices[prices.size() - period - 1];
    
    if (past_price == 0.0) return 0.0;
    
    return ((current_price - past_price) / past_price) * 100.0;
}

// Momentum Score (combination of multiple indicators)
double Indicators::momentum_score(const std::deque<double>& prices, size_t short_period, size_t long_period) {
    if (prices.size() < long_period) return 0.0;
    
    double short_sma = simple_moving_average(prices, short_period);
    double long_sma = simple_moving_average(prices, long_period);
    double current_price = prices.back();
    
    // Price above short-term MA: +1, below: -1
    double price_vs_short = (current_price > short_sma) ? 1.0 : -1.0;
    
    // Short-term MA above long-term MA: +1, below: -1
    double ma_trend = (short_sma > long_sma) ? 1.0 : -1.0;
    
    // Price change momentum
    double price_change = price_change_percent(prices, short_period);
    double momentum_factor = std::tanh(price_change / 10.0); // Normalize to [-1, 1]
    
    // Combined momentum score
    return (price_vs_short + ma_trend + momentum_factor) / 3.0;
} 