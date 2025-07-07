#pragma once
#include <chrono>
#include <atomic>

class PerformanceMonitor {
public:
    PerformanceMonitor();
    void start();
    void stop();
    void record_event();
    double get_events_per_second() const;
private:
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point end_time_;
    std::atomic<uint64_t> event_count_{0};
    std::atomic<bool> running_{false};
}; 