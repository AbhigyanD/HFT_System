#pragma once
#include <vector>
#include <deque>
#include <cmath>

class Indicators {
public:
    static double simple_moving_average(const std::deque<double>& values, size_t period);
    static double relative_strength_index(const std::deque<double>& prices, size_t period);
    static std::pair<double, double> macd(const std::deque<double>& prices, size_t fast_period, size_t slow_period, size_t signal_period);
    static double price_change_percent(const std::deque<double>& prices, size_t period);
    static double momentum_score(const std::deque<double>& prices, size_t short_period, size_t long_period);
}; 