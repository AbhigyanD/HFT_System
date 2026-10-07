#include "strategy_factory.h"
#include "strategy.h"
#include "mean_reversion_strategy.h"

StrategyHandle make_strategy(const StrategyParams& p) {
    StrategyHandle h;
    if (p.kind == StrategyKind::Momentum) {
        StrategyConfig c;
        c.momentum_threshold = p.momentum_threshold;
        c.short_period = p.short_period;
        c.long_period = p.long_period;
        c.position_size = p.position_size;
        c.stop_loss_pct = p.stop_loss_pct;
        c.take_profit_pct = p.take_profit_pct;
        auto engine = std::make_shared<StrategyEngine>(c);
        h.name = "Momentum";
        h.generate = [engine](const std::vector<std::shared_ptr<Order>>& o) { return engine->generate_signals(o); };
        h.reset = [engine]() { engine->reset_position(); };
        h.last_reason = [engine]() { return engine->get_last_signal_reason(); };
    } else {
        MeanReversionConfig c;
        c.lookback_period = p.lookback_period;
        c.entry_threshold_pct = p.entry_threshold_pct / 100.0;
        c.exit_threshold_pct = p.exit_threshold_pct / 100.0;
        c.position_size = p.position_size;
        c.stop_loss_pct = p.stop_loss_pct;
        c.take_profit_pct = p.take_profit_pct;
        auto engine = std::make_shared<MeanReversionEngine>(c);
        h.name = "Mean Reversion";
        h.generate = [engine](const std::vector<std::shared_ptr<Order>>& o) { return engine->generate_signals(o); };
        h.reset = [engine]() { engine->reset_position(); };
        h.last_reason = [engine]() { return engine->get_last_signal_reason(); };
    }
    return h;
}
