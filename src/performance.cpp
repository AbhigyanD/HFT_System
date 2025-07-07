#include "performance.h"

PerformanceMonitor::PerformanceMonitor() {}

void PerformanceMonitor::start() {
    running_ = true;
    event_count_ = 0;
    start_time_ = std::chrono::steady_clock::now();
}

void PerformanceMonitor::stop() {
    running_ = false;
    end_time_ = std::chrono::steady_clock::now();
}

void PerformanceMonitor::record_event() {
    if (running_) event_count_++;
}

double PerformanceMonitor::get_events_per_second() const {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time_ - start_time_).count();
    if (duration <= 0) return 0.0;
    return static_cast<double>(event_count_.load()) / duration;
} 