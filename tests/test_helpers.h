#pragma once
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include "order_book.h"

// Minimal test harness: CHECK records a failure and continues; main returns the failure count.
inline int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

inline std::shared_ptr<Order> make_tick(double price) {
    auto o = std::make_shared<Order>();
    o->order_id = 1;
    o->side = OrderSide::BUY;
    o->price = static_cast<Price>(price * 100.0 + 0.5);
    o->quantity = 1;
    o->type = OrderType::MARKET;
    o->timestamp = std::chrono::steady_clock::now();
    return o;
}

// Feed a price series to a signal function; returns the sides emitted per bar (+1 buy, -1 sell, 0 none).
template <class Fn>
std::vector<int> run_series(Fn&& fn, const std::vector<double>& prices) {
    std::vector<int> sides;
    for (double p : prices) {
        auto orders = fn(std::vector<std::shared_ptr<Order>>{make_tick(p)});
        sides.push_back(orders.empty() ? 0 : (orders[0]->side == OrderSide::BUY ? 1 : -1));
    }
    return sides;
}

inline std::vector<double> ramp(double from, double to, int n) {
    std::vector<double> v;
    for (int i = 0; i < n; ++i) v.push_back(from + (to - from) * i / (n - 1));
    return v;
}
inline std::vector<double> concat(std::vector<double> a, const std::vector<double>& b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}
inline std::vector<double> flat(double p, int n) { return std::vector<double>(n, p); }
