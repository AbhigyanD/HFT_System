#pragma once
#include <QMainWindow>
#include <QThread>
#include <QVector>
#include "plot_widget.h"
#include "sim_worker.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void startSim(StrategyParams params, double volatilityPct, int tickMs, double startPrice);
    void stopSim();

private slots:
    void toggleRun();
    void onTick(int index, double price);
    void onTrade(bool buy, int index, double price, double qty, double pnlPct, double pnlAbs, QString reason);
    void onAccount(double equity, double cash, double position, double realizedPnl, int trades, int wins);
    void runBacktest();
    void exportTrades();

private:
    QWidget* buildSidebar();
    QWidget* buildLiveTab();
    QWidget* buildBacktestTab();
    StrategyParams currentParams() const;
    void setRunning(bool running);
    QLabel* makeKpi(QWidget* parent, class QGridLayout* grid, int col, const QString& title);

    // Sidebar
    QComboBox* strategyBox_;
    QStackedWidget* paramStack_;
    QSpinBox *shortPeriod_, *longPeriod_, *lookback_;
    QDoubleSpinBox *momThreshold_, *entryPct_, *exitPct_, *stopLoss_, *takeProfit_, *posSize_;

    // Live
    QPushButton* runButton_;
    QDoubleSpinBox *liveVol_, *startPrice_;
    QSpinBox* tickMs_;
    QLabel *kPrice_, *kEquity_, *kPnl_, *kPosition_, *kTrades_, *kWinRate_;
    PlotWidget* pricePlot_;
    QTableWidget* tradeTable_;
    QVector<double> prices_;
    QVector<PlotWidget::Marker> markers_;
    static constexpr int kMaxPoints = 600;
    int firstIndex_ = 0;  // global index of prices_[0]

    // Backtest
    QSpinBox *btBars_, *btSeed_;
    QDoubleSpinBox *btVol_, *btDrift_;
    QLabel *bReturn_, *bBuyHold_, *bTrades_, *bWin_, *bDd_, *bSharpe_;
    PlotWidget* equityPlot_;

    QThread thread_;
    SimWorker* worker_;
    bool running_ = false;
};
