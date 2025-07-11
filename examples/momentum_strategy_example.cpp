#include "../src/order_book.h"
#include "../src/matching_engine.h"
#include "../src/market_data.h"
#include "../src/strategy.h"
#include "../src/risk.h"
#include "../src/indicators.h"
#include "../src/performance.h"
#include "../src/threading.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

/**
 * Momentum Strategy Example
 * 
 * This example demonstrates how to:
 * 1. Configure a momentum strategy with different parameters
 * 2. Run the strategy and observe real-time signals
 * 3. Analyze the performance and P&L
 * 4. Customize the strategy for different market conditions
 */

void run_strategy_with_config(const StrategyConfig& config, const std::string& strategy_name);

void print_strategy_stats(const StrategyEngine& strategy, const MatchingEngine& engine, 
                         const PerformanceMonitor& perf, int elapsed_seconds) {
    std::cout << "\n📊 Strategy Statistics (" << elapsed_seconds << "s elapsed):" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    auto config = strategy.get_config();
    std::cout << "Strategy Configuration:" << std::endl;
    std::cout << "  Momentum Threshold: " << config.momentum_threshold << std::endl;
    std::cout << "  RSI Levels: " << config.rsi_oversold << " - " << config.rsi_overbought << std::endl;
    std::cout << "  MA Periods: " << config.short_period << "/" << config.long_period << std::endl;
    std::cout << "  Position Size: " << config.position_size << std::endl;
    std::cout << "  Risk Management: " << config.stop_loss_pct << "% / " << config.take_profit_pct << "%" << std::endl;
    
    std::cout << "\nPerformance Metrics:" << std::endl;
    std::cout << "  Processed Orders: " << engine.get_processed_orders() << std::endl;
    std::cout << "  Matched Trades: " << engine.get_matched_trades() << std::endl;
    std::cout << "  Events/sec: " << std::fixed << std::setprecision(1) << perf.get_events_per_second() << std::endl;
    std::cout << "  Avg Processing Time: " << engine.get_average_processing_time_ns() << " ns" << std::endl;
    
    std::cout << "\nStrategy State:" << std::endl;
    std::cout << "  Price History Size: " << strategy.get_price_history_size() << std::endl;
    std::cout << "  In Position: " << (strategy.is_in_position() ? "Yes" : "No") << std::endl;
    if (strategy.is_in_position()) {
        std::cout << "  Entry Price: " << strategy.get_entry_price() << std::endl;
    }
    
    auto [best_bid, best_ask] = engine.get_best_bid_ask();
    std::cout << "  Current Spread: " << std::fixed << std::setprecision(2) 
              << (best_ask - best_bid) / 100.0 << std::endl;
}

void run_conservative_strategy() {
    std::cout << "\n🎯 Running Conservative Momentum Strategy" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Conservative configuration - fewer signals, tighter risk management
    StrategyConfig config;
    config.momentum_threshold = 0.4;    // Higher threshold = fewer signals
    config.rsi_oversold = 20.0;         // More conservative RSI levels
    config.rsi_overbought = 80.0;
    config.short_period = 10;            // Longer periods = less noise
    config.long_period = 30;
    config.position_size = 25.0;        // Smaller position size
    config.stop_loss_pct = 1.0;         // Tighter stop loss
    config.take_profit_pct = 2.0;       // Lower take profit
    
    run_strategy_with_config(config, "Conservative");
}

void run_aggressive_strategy() {
    std::cout << "\n⚡ Running Aggressive Momentum Strategy" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    // Aggressive configuration - more signals, higher risk tolerance
    StrategyConfig config;
    config.momentum_threshold = 0.15;   // Lower threshold = more signals
    config.rsi_oversold = 30.0;         // Less conservative RSI levels
    config.rsi_overbought = 70.0;
    config.short_period = 3;             // Shorter periods = faster response
    config.long_period = 15;
    config.position_size = 75.0;        // Larger position size
    config.stop_loss_pct = 2.5;         // Wider stop loss
    config.take_profit_pct = 4.0;       // Higher take profit
    
    run_strategy_with_config(config, "Aggressive");
}

void run_strategy_with_config(const StrategyConfig& config, const std::string& strategy_name) {
    // Initialize components
    MatchingEngine engine;
    MarketData market_data;
    StrategyEngine strategy(config);
    RiskManager risk;
    PerformanceMonitor perf;
    ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<bool> running{true};
    
    std::cout << "Starting " << strategy_name << " strategy..." << std::endl;
    std::cout << "Press Enter to stop after 30 seconds..." << std::endl;
    
    perf.start();
    auto start_time = std::chrono::steady_clock::now();
    
    // Set up market data processing
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
    
    // Run for 30 seconds with status updates
    int update_counter = 0;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        update_counter++;
        
        // Status update every 5 seconds
        if (update_counter >= 50) {
            update_counter = 0;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            
            print_strategy_stats(strategy, engine, perf, elapsed.count());
            
            // Stop after 30 seconds
            if (elapsed.count() >= 30) {
                running = false;
            }
        }
        
        // Check for user input
        if (std::cin.rdbuf()->in_avail()) {
            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) {
                running = false;
            }
        }
    }
    
    // Shutdown
    market_data.stop();
    pool.shutdown();
    perf.stop();
    
    // Final statistics
    auto final_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time);
    
    std::cout << "\n🏁 " << strategy_name << " Strategy Complete!" << std::endl;
    std::cout << "=========================================" << std::endl;
    print_strategy_stats(strategy, engine, perf, final_elapsed.count());
}

int main() {
    std::cout << "🚀 Momentum Strategy Examples" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "This example demonstrates different momentum strategy configurations." << std::endl;
    std::cout << "Each strategy runs for 30 seconds to show performance differences." << std::endl;
    
    // Run conservative strategy
    run_conservative_strategy();
    
    std::cout << "\nPress Enter to continue to aggressive strategy..." << std::endl;
    std::cin.get();
    
    // Run aggressive strategy
    run_aggressive_strategy();
    
    std::cout << "\n✅ All examples complete!" << std::endl;
    std::cout << "Compare the results to see how different configurations affect:" << std::endl;
    std::cout << "- Number of signals generated" << std::endl;
    std::cout << "- Risk management effectiveness" << std::endl;
    std::cout << "- Overall system performance" << std::endl;
    
    return 0;
} 