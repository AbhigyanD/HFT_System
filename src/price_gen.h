#pragma once
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

// Geometric random walk. `drift` and `volatility` are per-bar log-return parameters.
inline std::vector<double> generate_prices(size_t n, double start = 100.0, double drift = 0.0001,
                                           double volatility = 0.02, uint32_t seed = 42) {
    std::vector<double> prices;
    prices.reserve(n);
    std::mt19937 rng(seed);
    std::normal_distribution<double> normal(0, 1);
    double s = start;
    for (size_t i = 0; i < n; ++i) {
        s = s * std::exp(drift + volatility * normal(rng));
        prices.push_back(s);
    }
    return prices;
}
