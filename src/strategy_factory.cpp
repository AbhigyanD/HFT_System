#include "strategy_factory.h"
#include "strategy.h"
#include "mean_reversion_strategy.h"
#include "extra_strategies.h"

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
    } else if (p.kind == StrategyKind::MeanReversion) {
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
    } else {
        RuleRiskConfig r{p.position_size, p.stop_loss_pct, p.take_profit_pct};
        std::shared_ptr<RuleStrategy> engine;
        switch (p.kind) {
            case StrategyKind::Bollinger: engine = std::make_shared<BollingerStrategy>(r, p.lookback_period, p.band_std); break;
            case StrategyKind::Rsi: engine = std::make_shared<RsiStrategy>(r, p.lookback_period, p.rsi_oversold, p.rsi_exit); break;
            case StrategyKind::Breakout: engine = std::make_shared<BreakoutStrategy>(r, p.lookback_period); break;
            case StrategyKind::Macd: engine = std::make_shared<MacdStrategy>(r, p.macd_fast, p.macd_slow, p.macd_signal); break;
            case StrategyKind::Stochastic: engine = std::make_shared<StochasticStrategy>(r, p.lookback_period, p.stoch_oversold, p.stoch_exit); break;
            case StrategyKind::EmaCross: engine = std::make_shared<EmaCrossStrategy>(r, p.ema_fast, p.ema_slow); break;
            default: engine = std::make_shared<RocStrategy>(r, p.lookback_period, p.roc_threshold_pct); break;
        }
        h.name = strategy_name(p.kind);
        h.generate = [engine](const std::vector<std::shared_ptr<Order>>& o) { return engine->generate_signals(o); };
        h.reset = [engine]() { engine->reset_position(); };
        h.last_reason = [engine]() { return engine->get_last_signal_reason(); };
    }
    return h;
}
