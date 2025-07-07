#include "indicators.h"
#include <numeric>

// Simple Moving Average

double Indicators::simple_moving_average(const std::deque<double>& values, size_t period) {
    if (values.size() < period || period == 0) return 0.0;
    double sum = std::accumulate(values.end() - period, values.end(), 0.0);
    return sum / period;
} 