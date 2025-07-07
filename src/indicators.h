#pragma once
#include <vector>
#include <deque>

class Indicators {
public:
    static double simple_moving_average(const std::deque<double>& values, size_t period);
}; 