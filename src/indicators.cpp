#include "indicators.h"
#include <numeric>
#include <algorithm>

// Simple Moving Average
double Indicators::simple_moving_average(const std::deque<double>& values, size_t period) {
    if (values.size() < period || period == 0) return 0.0;
    double sum = std::accumulate(values.end() - period, values.end(), 0.0);
    return sum / period;
}

// Relative Strength Index
double Indicators::relative_strength_index(const std::deque<double>& prices, size_t period) {
    if (prices.size() < period + 1) return 50.0; // Neutral RSI
    
    std::deque<double> gains, losses;
    for (size_t i = 1; i < prices.size(); ++i) {
        double change = prices[i] - prices[i-1];
        if (change > 0) {
            gains.push_back(change);
            losses.push_back(0.0);
        } else {
            gains.push_back(0.0);
            losses.push_back(-change);
        }
    }
    
    if (gains.size() < period) return 50.0;
    
    double avg_gain = std::accumulate(gains.end() - period, gains.end(), 0.0) / period;
    double avg_loss = std::accumulate(losses.end() - period, losses.end(), 0.0) / period;
    
    if (avg_loss == 0.0) return 100.0;
    
    double rs = avg_gain / avg_loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

// MACD (Moving Average Convergence Divergence)
std::pair<double, double> Indicators::macd(const std::deque<double>& prices, size_t fast_period, size_t slow_period, size_t signal_period) {
    if (prices.size() < slow_period) return {0.0, 0.0};
    
    double fast_ema = simple_moving_average(prices, fast_period);
    double slow_ema = simple_moving_average(prices, slow_period);
    
    double macd_line = fast_ema - slow_ema;
    
    // For simplicity, using SMA as signal line (in practice, EMA would be better)
    std::deque<double> macd_values;
    for (size_t i = slow_period; i < prices.size(); ++i) {
        std::deque<double> temp_prices(prices.begin(), prices.begin() + i + 1);
        double fast = simple_moving_average(temp_prices, fast_period);
        double slow = simple_moving_average(temp_prices, slow_period);
        macd_values.push_back(fast - slow);
    }
    
    double signal_line = (macd_values.size() >= signal_period) ? 
        simple_moving_average(macd_values, signal_period) : 0.0;
    
    return {macd_line, signal_line};
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