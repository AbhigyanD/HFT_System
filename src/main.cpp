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
#include <iomanip>
#include <chrono>

namespace {

void print_strategy_config(const StrategyEngine& strategy) {
    auto config = strategy.get_config();
    std::cout << "Momentum Strategy Configuration\n"
              << "  momentum_threshold=" << config.momentum_threshold
              << " rsi_oversold=" << config.rsi_oversold
              << " rsi_overbought=" << config.rsi_overbought
              << " short_period=" << config.short_period
              << " long_period=" << config.long_period
              << " position_size=" << config.position_size
              << " stop_loss_pct=" << config.stop_loss_pct
              << " take_profit_pct=" << config.take_profit_pct << "\n";
}

void print_strategy_status(const StrategyEngine& strategy) {
    std::cout << "  price_history_size=" << strategy.get_price_history_size()
              << " in_position=" << (strategy.is_in_position() ? 1 : 0);
    if (strategy.is_in_position()) {
        std::cout << " entry_price=" << std::fixed << std::setprecision(2) << strategy.get_entry_price();
    }
    std::cout << "\n";
}

}  // namespace

int main() {
    std::cout << "NanoEX HFT System starting.\n";

    MatchingEngine engine;
    MarketData market_data;
    RiskManager risk;
    PerformanceMonitor perf;
    ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<bool> running{true};

    StrategyConfig strategy_config;
    strategy_config.momentum_threshold = 0.25;
    strategy_config.rsi_oversold = 25.0;
    strategy_config.rsi_overbought = 75.0;
    strategy_config.short_period = 5;
    strategy_config.long_period = 20;
    strategy_config.position_size = 50.0;
    strategy_config.stop_loss_pct = 1.5;
    strategy_config.take_profit_pct = 3.0;

    StrategyEngine strategy(strategy_config);
    print_strategy_config(strategy);

    perf.start();

    market_data.start([&](const std::vector<std::shared_ptr<Order>>& market_orders) {
        pool.enqueue([&]() {
            auto signals = strategy.generate_signals(market_orders);

            if (strategy.get_last_signal_type() != SignalType::HOLD) {
                if (strategy.get_last_signal_type() == SignalType::BUY) {
                    std::cout << "BUY Signal: " << strategy.get_last_signal_reason()
                              << " (Confidence: " << std::fixed << std::setprecision(2)
                              << strategy.get_last_signal_confidence() * 100 << "%)\n";
                } else {
                    std::cout << "SELL Signal: " << strategy.get_last_signal_reason()
                              << " (Confidence: " << std::fixed << std::setprecision(2)
                              << strategy.get_last_signal_confidence() * 100
                              << "%, P&L: " << strategy.get_last_signal_pnl_pct() << "%)\n";
                }
            }

            auto filtered = risk.filter_orders(signals);

            for (const auto& order : filtered) {
                engine.add_order(order);
                perf.record_event();
                std::cout << "Order: " << (order->side == OrderSide::BUY ? "BUY" : "SELL")
                          << " @ " << std::fixed << std::setprecision(2) << (order->price / 100.0)
                          << " x " << order->quantity << "\n";
            }
        });
    });

    auto start_time = std::chrono::steady_clock::now();
    int update_counter = 0;

    std::cout << "Running. Press Enter to stop. Status every 5s.\n";

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++update_counter;
        if (update_counter >= 50) {
            update_counter = 0;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            auto [best_bid, best_ask] = engine.get_best_bid_ask();
            std::cout << "Status " << elapsed.count() << "s | orders=" << engine.get_processed_orders()
                      << " trades=" << engine.get_matched_trades()
                      << " events/s=" << std::fixed << std::setprecision(1) << perf.get_events_per_second()
                      << " avg_ns=" << engine.get_average_processing_time_ns()
                      << " bid=" << (best_bid / 100.0) << " ask=" << (best_ask / 100.0) << "\n";
        }
        if (std::cin.rdbuf()->in_avail()) {
            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) running = false;
        }
    }

    std::cout << "Shutting down.\n";
    market_data.stop();
    pool.shutdown();
    perf.stop();

    std::cout << "Final: orders=" << engine.get_processed_orders()
              << " trades=" << engine.get_matched_trades()
              << " events/s=" << std::fixed << std::setprecision(1) << perf.get_events_per_second()
              << " avg_ns=" << engine.get_average_processing_time_ns() << "\n";
    auto [final_bid, final_ask] = engine.get_best_bid_ask();
    std::cout << "Best bid=" << (final_bid / 100.0) << " best_ask=" << (final_ask / 100.0) << "\n";
    print_strategy_config(strategy);
    print_strategy_status(strategy);
    if (risk.get_orders_rejected() > 0) {
        std::cout << "Risk rejected " << risk.get_orders_rejected() << " orders.\n";
    }
    std::cout << "Shutdown complete.\n";
    return 0;
}
