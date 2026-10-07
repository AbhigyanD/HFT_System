#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include <random>
#include "strategy_factory.h"

Q_DECLARE_METATYPE(StrategyParams)

// Runs a paper-trading simulation on its own thread: random-walk market, selected strategy,
// simple long-only paper account. All results leave via signals.
class SimWorker : public QObject {
    Q_OBJECT
public:
    static constexpr double kInitialCash = 100000.0;
    explicit SimWorker(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void start(StrategyParams params, double volatilityPct, int tickMs, double startPrice);
    void stop();

signals:
    void tick(int index, double price);
    void trade(bool buy, int index, double price, double qty, double pnlPct, double pnlAbs, QString reason);
    void account(double equity, double cash, double position, double realizedPnl, int trades, int wins);

private:
    void onTick();

    std::unique_ptr<QTimer> timer_;
    StrategyHandle strategy_;
    std::mt19937 rng_{std::random_device{}()};
    double volatility_ = 0.001;
    double price_ = 100.0;
    int index_ = 0;
    double cash_ = kInitialCash, position_ = 0, entry_ = 0, realized_ = 0;
    int trades_ = 0, wins_ = 0;
    uint64_t nextId_ = 1;
};
