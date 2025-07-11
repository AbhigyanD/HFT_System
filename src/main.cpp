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

void print_strategy_info(const StrategyEngine& strategy) {
    std::cout << "\n=== Momentum Strategy Configuration ===" << std::endl;
    auto config = strategy.get_config();
    std::cout << "Momentum Threshold: " << config.momentum_threshold << std::endl;
    std::cout << "RSI Oversold: " << config.rsi_oversold << std::endl;
    std::cout << "RSI Overbought: " << config.rsi_overbought << std::endl;
    std::cout << "Short Period: " << config.short_period << std::endl;
    std::cout << "Long Period: " << config.long_period << std::endl;
    std::cout << "Position Size: " << config.position_size << std::endl;
    std::cout << "Stop Loss: " << config.stop_loss_pct << "%" << std::endl;
    std::cout << "Take Profit: " << config.take_profit_pct << "%" << std::endl;
    std::cout << "Price History Size: " << strategy.get_price_history_size() << std::endl;
    std::cout << "In Position: " << (strategy.is_in_position() ? "Yes" : "No") << std::endl;
    if (strategy.is_in_position()) {
        std::cout << "Entry Price: " << strategy.get_entry_price() << std::endl;
    }
    std::cout << "=====================================\n" << std::endl;
}

int main() {
    std::cout << "🚀 Starting HFT System with Momentum Strategy" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Initialize components
    MatchingEngine engine;
    MarketData market_data;
    RiskManager risk;
    PerformanceMonitor perf;
    ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<bool> running{true};
    
    // Configure momentum strategy
    StrategyConfig config;
    config.momentum_threshold = 0.25;  // Lower threshold for more signals
    config.rsi_oversold = 25.0;        // More conservative RSI levels
    config.rsi_overbought = 75.0;
    config.short_period = 5;            // Shorter periods for faster response
    config.long_period = 20;
    config.position_size = 50.0;       // Smaller position size for demo
    config.stop_loss_pct = 1.5;        // Tighter risk management
    config.take_profit_pct = 3.0;
    
    StrategyEngine strategy(config);
    
    // Print initial strategy configuration
    print_strategy_info(strategy);
    
    // Start performance monitoring
    perf.start();
    
    // Set up market data processing with strategy
    market_data.start([&](const std::vector<std::shared_ptr<Order>>& market_orders) {
        pool.enqueue([&]() {
            // Generate strategy signals
            auto signals = strategy.generate_signals(market_orders);
            
            // Apply risk management
            auto filtered = risk.filter_orders(signals);
            
            // Execute orders
            for (auto& order : filtered) {
                engine.add_order(order);
                perf.record_event();
                
                // Print order details
                std::cout << "📊 Order: " << (order->side == OrderSide::BUY ? "BUY" : "SELL")
                          << " @ " << std::fixed << std::setprecision(2) << (order->price / 100.0)
                          << " x " << order->quantity << std::endl;
            }
        });
    });
    
    // Main loop with periodic status updates
    auto start_time = std::chrono::steady_clock::now();
    int update_counter = 0;
    
    std::cout << "HFT System running. Press Enter to stop..." << std::endl;
    std::cout << "Status updates every 5 seconds..." << std::endl;
    
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Periodic status updates
        update_counter++;
        if (update_counter >= 50) { // ~5 seconds
            update_counter = 0;
            
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            
            std::cout << "\n📈 System Status (" << elapsed.count() << "s elapsed):" << std::endl;
            std::cout << "Processed Orders: " << engine.get_processed_orders() << std::endl;
            std::cout << "Matched Trades: " << engine.get_matched_trades() << std::endl;
            std::cout << "Events/sec: " << std::fixed << std::setprecision(1) << perf.get_events_per_second() << std::endl;
            std::cout << "Avg Processing Time: " << engine.get_average_processing_time_ns() << " ns" << std::endl;
            
            auto [best_bid, best_ask] = engine.get_best_bid_ask();
            std::cout << "Best Bid: " << std::fixed << std::setprecision(2) << best_bid 
                      << " | Best Ask: " << best_ask << std::endl;
            
            print_strategy_info(strategy);
        }
        
        // Check for user input (non-blocking)
        if (std::cin.rdbuf()->in_avail()) {
            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) {
                running = false;
            }
        }
    }
    
    // Shutdown
    std::cout << "\n🛑 Shutting down HFT System..." << std::endl;
    market_data.stop();
    pool.shutdown();
    perf.stop();
    
    // Final statistics
    std::cout << "\n📊 Final Statistics:" << std::endl;
    std::cout << "===================" << std::endl;
    std::cout << "Total Processed Orders: " << engine.get_processed_orders() << std::endl;
    std::cout << "Total Matched Trades: " << engine.get_matched_trades() << std::endl;
    std::cout << "Average Events/sec: " << std::fixed << std::setprecision(1) << perf.get_events_per_second() << std::endl;
    std::cout << "Average Order Processing Time: " << engine.get_average_processing_time_ns() << " ns" << std::endl;
    
    auto [final_bid, final_ask] = engine.get_best_bid_ask();
    std::cout << "Final Best Bid: " << std::fixed << std::setprecision(2) << final_bid << std::endl;
    std::cout << "Final Best Ask: " << final_ask << std::endl;
    
    print_strategy_info(strategy);
    
    std::cout << "\n✅ HFT System shutdown complete." << std::endl;
    return 0;
} 