#pragma once
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include "order_book.h"

// Long-only strategies sharing price-history handling and stop-loss / take-profit exits.
struct RuleRiskConfig {
    double position_size = 50.0;
    double stop_loss_pct = 2.0;    // percent
    double take_profit_pct = 4.0;  // percent
};

class RuleStrategy {
public:
    explicit RuleStrategy(const RuleRiskConfig& risk) : risk_(risk) {}
    virtual ~RuleStrategy() = default;

    std::vector<std::shared_ptr<Order>> generate_signals(const std::vector<std::shared_ptr<Order>>& market_orders);

    bool is_in_position() const { return in_position_; }
    void reset_position();
    const std::string& get_last_signal_reason() const { return last_reason_; }

protected:
    // Return true (and fill `reason`) when the strategy wants to enter / exit now.
    virtual bool wants_entry(const std::deque<double>& prices, std::string& reason) = 0;
    virtual bool wants_exit(const std::deque<double>& prices, std::string& reason) = 0;
    virtual void on_reset() {}

private:
    RuleRiskConfig risk_;
    std::deque<double> prices_;
    bool in_position_ = false;
    double entry_price_ = 0.0;
    std::string last_reason_;
};

// Buy when price closes below the lower Bollinger band, sell when it returns to the middle band.
class BollingerStrategy : public RuleStrategy {
public:
    BollingerStrategy(const RuleRiskConfig& r, size_t period, double num_std) : RuleStrategy(r), period_(period), k_(num_std) {}
protected:
    bool wants_entry(const std::deque<double>& p, std::string& reason) override;
    bool wants_exit(const std::deque<double>& p, std::string& reason) override;
private:
    bool bands(const std::deque<double>& p, double& mid, double& sd) const;
    size_t period_;
    double k_;
};

// Buy when RSI drops below the oversold level, sell when it recovers above the exit level.
class RsiStrategy : public RuleStrategy {
public:
    RsiStrategy(const RuleRiskConfig& r, size_t period, double oversold, double exit_level)
        : RuleStrategy(r), period_(period), oversold_(oversold), exit_(exit_level) {}
protected:
    bool wants_entry(const std::deque<double>& p, std::string& reason) override;
    bool wants_exit(const std::deque<double>& p, std::string& reason) override;
private:
    size_t period_;
    double oversold_, exit_;
};

// Donchian channel breakout: buy a new N-bar high, sell when price drops under the N/2-bar low.
class BreakoutStrategy : public RuleStrategy {
public:
    BreakoutStrategy(const RuleRiskConfig& r, size_t period) : RuleStrategy(r), period_(period) {}
protected:
    bool wants_entry(const std::deque<double>& p, std::string& reason) override;
    bool wants_exit(const std::deque<double>& p, std::string& reason) override;
private:
    size_t period_;
};

// MACD line crossing its signal line.
class MacdStrategy : public RuleStrategy {
public:
    MacdStrategy(const RuleRiskConfig& r, size_t fast, size_t slow, size_t signal)
        : RuleStrategy(r), fast_(fast), slow_(slow), signal_(signal) {}
protected:
    bool wants_entry(const std::deque<double>& p, std::string& reason) override;
    bool wants_exit(const std::deque<double>& p, std::string& reason) override;
private:
    bool cross(const std::deque<double>& p, double& hist, double& prev_hist) const;
    size_t fast_, slow_, signal_;
};

// Rate-of-change trend following: buy when N-bar ROC exceeds the threshold, sell when it turns negative.
class RocStrategy : public RuleStrategy {
public:
    RocStrategy(const RuleRiskConfig& r, size_t period, double threshold_pct)
        : RuleStrategy(r), period_(period), threshold_(threshold_pct) {}
protected:
    bool wants_entry(const std::deque<double>& p, std::string& reason) override;
    bool wants_exit(const std::deque<double>& p, std::string& reason) override;
private:
    size_t period_;
    double threshold_;
};
