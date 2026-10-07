#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include "paper_sim.h"

Q_DECLARE_METATYPE(StrategyParams)

// Runs a PaperSim on its own thread and forwards results as Qt signals.
class SimWorker : public QObject {
    Q_OBJECT
public:
    static constexpr double kInitialCash = PaperSim::kInitialCash;
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
    void publish(const PaperSim::Step& s);

    std::unique_ptr<QTimer> timer_;
    PaperSim sim_;
};
