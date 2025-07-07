#include "order_book.h"
#include "matching_engine.h"
#include "market_data.h"
#include "strategy.h"
#include "risk.h"
#include "indicators.h"
#include "performance.h"
#include "threading.h"
#include <iostream>
#include <atomic>

int main() {
    MatchingEngine engine;
    MarketData market_data;
    StrategyEngine strategy;
    RiskManager risk;
    PerformanceMonitor perf;
    ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<bool> running{true};

    perf.start();
    market_data.start([&](const std::vector<std::shared_ptr<Order>>& market_orders) {
        pool.enqueue([&]() {
            auto signals = strategy.generate_signals(market_orders);
            auto filtered = risk.filter_orders(signals);
            for (auto& order : filtered) {
                engine.add_order(order);
                perf.record_event();
            }
        });
    });

    std::cout << "HFT System running. Press Enter to stop..." << std::endl;
    std::cin.get();
    running = false;
    market_data.stop();
    pool.shutdown();
    perf.stop();

    std::cout << "Processed orders: " << engine.get_processed_orders() << std::endl;
    std::cout << "Matched trades: " << engine.get_matched_trades() << std::endl;
    std::cout << "Events/sec: " << perf.get_events_per_second() << std::endl;
    std::cout << "Average order processing time (ns): " << engine.get_average_processing_time_ns() << std::endl;
    auto [best_bid, best_ask] = engine.get_best_bid_ask();
    std::cout << "Best Bid: " << best_bid << " Best Ask: " << best_ask << std::endl;
    return 0;
} 