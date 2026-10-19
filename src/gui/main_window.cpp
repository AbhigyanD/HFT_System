#include <algorithm>
#include "main_window.h"
#include "backtester.h"
#include "price_gen.h"
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
const char* kGreen = "#2ec46b";
const char* kRed = "#f04d4d";

QDoubleSpinBox* dspin(double lo, double hi, double v, double step, int dec, const QString& suffix = {}) {
    auto* s = new QDoubleSpinBox;
    s->setRange(lo, hi);
    s->setValue(v);
    s->setSingleStep(step);
    s->setDecimals(dec);
    s->setSuffix(suffix);
    return s;
}

QSpinBox* ispin(int lo, int hi, int v, const QString& suffix = {}) {
    auto* s = new QSpinBox;
    s->setRange(lo, hi);
    s->setValue(v);
    s->setSuffix(suffix);
    return s;
}

void colorize(QLabel* l, double v) {
    l->setStyleSheet(QString("font-size:20px; font-weight:600;%1")
                         .arg(v > 0 ? QString(" color:%1;").arg(kGreen) : v < 0 ? QString(" color:%1;").arg(kRed) : QString()));
}
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("NanoEX — Strategy Simulator & Backtester");
    resize(1280, 800);

    auto* central = new QWidget;
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(buildSidebar());

    auto* tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    tabs->addTab(buildLiveTab(), "Live Simulation");
    tabs->addTab(buildBacktestTab(), "Backtest");
    root->addWidget(tabs, 1);
    setCentralWidget(central);

    // Engine runs on its own thread; everything crosses by queued signals.
    qRegisterMetaType<StrategyParams>("StrategyParams");
    worker_ = new SimWorker;
    worker_->moveToThread(&thread_);
    connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(this, &MainWindow::startSim, worker_, &SimWorker::start);
    connect(this, &MainWindow::stopSim, worker_, &SimWorker::stop, Qt::BlockingQueuedConnection);
    connect(worker_, &SimWorker::tick, this, &MainWindow::onTick);
    connect(worker_, &SimWorker::trade, this, &MainWindow::onTrade);
    connect(worker_, &SimWorker::account, this, &MainWindow::onAccount);
    thread_.start();

    statusBar()->showMessage("Ready. Pick a strategy, then press Start.");
}

MainWindow::~MainWindow() {
    if (running_) emit stopSim();
    thread_.quit();
    thread_.wait();
}

QWidget* MainWindow::buildSidebar() {
    auto* side = new QWidget;
    side->setFixedWidth(290);
    side->setObjectName("sidebar");
    auto* v = new QVBoxLayout(side);
    v->setContentsMargins(16, 16, 16, 16);
    v->setSpacing(12);

    auto* title = new QLabel("<b style='font-size:18px'>NanoEX</b><br><span style='color:gray'>Strategy lab</span>");
    v->addWidget(title);

    auto* sg = new QGroupBox("Strategy");
    auto* sl = new QVBoxLayout(sg);
    strategyBox_ = new QComboBox;
    strategyBox_->addItems({"Momentum", "Mean Reversion", "Bollinger Bands", "RSI Reversal", "Breakout", "MACD Crossover", "Rate of Change", "Stochastic", "EMA Crossover", "Z-Score"});
    sl->addWidget(strategyBox_);

    paramStack_ = new QStackedWidget;
    auto* mom = new QWidget;
    auto* mf = new QFormLayout(mom);
    mf->setContentsMargins(0, 0, 0, 0);
    shortPeriod_ = ispin(2, 200, 5, " bars");
    longPeriod_ = ispin(3, 500, 20, " bars");
    momThreshold_ = dspin(0.0, 1.0, 0.25, 0.05, 2);
    mf->addRow("Short MA", shortPeriod_);
    mf->addRow("Long MA", longPeriod_);
    mf->addRow("Momentum ≥", momThreshold_);
    auto* mr = new QWidget;
    auto* rf = new QFormLayout(mr);
    rf->setContentsMargins(0, 0, 0, 0);
    lookback_ = ispin(2, 500, 20, " bars");
    entryPct_ = dspin(0.01, 20, 0.5, 0.1, 2, " %");
    exitPct_ = dspin(0.01, 20, 0.5, 0.1, 2, " %");
    rf->addRow("SMA lookback", lookback_);
    rf->addRow("Buy below SMA", entryPct_);
    rf->addRow("Sell above SMA", exitPct_);
    paramStack_->addWidget(mom);
    paramStack_->addWidget(mr);
    auto* gen = new QWidget;
    auto* gf = new QFormLayout(gen);
    gf->setContentsMargins(0, 0, 0, 0);
    window_ = ispin(2, 500, 20, " bars");
    gf->addRow("Window", window_);
    paramStack_->addWidget(gen);
    sl->addWidget(paramStack_);
    connect(strategyBox_, &QComboBox::currentIndexChanged, paramStack_,
            [this](int i) { paramStack_->setCurrentIndex(std::min(i, 2)); });
    v->addWidget(sg);

    auto* rg = new QGroupBox("Risk");
    auto* rl = new QFormLayout(rg);
    posSize_ = dspin(1, 100000, 50, 10, 0, " units");
    stopLoss_ = dspin(0.1, 50, 2.0, 0.5, 1, " %");
    takeProfit_ = dspin(0.1, 100, 4.0, 0.5, 1, " %");
    rl->addRow("Position size", posSize_);
    rl->addRow("Stop loss", stopLoss_);
    rl->addRow("Take profit", takeProfit_);
    v->addWidget(rg);

    auto* hint = new QLabel("Settings apply to both the live simulation and the backtest. "
                            "Changes take effect on the next Start / Run.");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:gray;");
    v->addWidget(hint);
    v->addStretch();
    return side;
}

QLabel* MainWindow::makeKpi(QWidget* parent, QGridLayout* grid, int col, const QString& title) {
    auto* card = new QFrame(parent);
    card->setObjectName("card");
    auto* l = new QVBoxLayout(card);
    l->setContentsMargins(14, 10, 14, 10);
    auto* t = new QLabel(title);
    t->setStyleSheet("color:gray; font-size:11px;");
    auto* val = new QLabel("—");
    val->setStyleSheet("font-size:20px; font-weight:600;");
    l->addWidget(t);
    l->addWidget(val);
    grid->addWidget(card, 0, col);
    return val;
}

QWidget* MainWindow::buildLiveTab() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(16, 16, 16, 16);
    v->setSpacing(12);

    auto* bar = new QHBoxLayout;
    runButton_ = new QPushButton("▶  Start");
    runButton_->setMinimumHeight(34);
    runButton_->setMinimumWidth(110);
    connect(runButton_, &QPushButton::clicked, this, &MainWindow::toggleRun);
    liveVol_ = dspin(0.01, 5, 0.15, 0.05, 2, " %/tick");
    tickMs_ = ispin(10, 2000, 100, " ms");
    startPrice_ = dspin(1, 100000, 100, 10, 2);
    bar->addWidget(runButton_);
    bar->addSpacing(16);
    bar->addWidget(new QLabel("Volatility"));
    bar->addWidget(liveVol_);
    bar->addWidget(new QLabel("Tick every"));
    bar->addWidget(tickMs_);
    bar->addWidget(new QLabel("Start price"));
    bar->addWidget(startPrice_);
    bar->addStretch();
    v->addLayout(bar);

    auto* kg = new QGridLayout;
    kg->setSpacing(10);
    kPrice_ = makeKpi(w, kg, 0, "Price");
    kEquity_ = makeKpi(w, kg, 1, "Equity");
    kPnl_ = makeKpi(w, kg, 2, "Realized P&L");
    kPosition_ = makeKpi(w, kg, 3, "Position");
    kTrades_ = makeKpi(w, kg, 4, "Closed trades");
    kWinRate_ = makeKpi(w, kg, 5, "Win rate");
    v->addLayout(kg);

    pricePlot_ = new PlotWidget;
    pricePlot_->setTitle("Price  (▲ buy  ▼ sell)");
    pricePlot_->setPlaceholder("Press Start to begin a simulated market");
    pricePlot_->setValuePrefix("$");
    v->addWidget(pricePlot_, 3);

    tradeTable_ = new QTableWidget(0, 6);
    tradeTable_->setHorizontalHeaderLabels({"Time", "Side", "Price", "Qty", "P&L", "Reason"});
    tradeTable_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    tradeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tradeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tradeTable_->verticalHeader()->hide();
    tradeTable_->setAlternatingRowColors(true);
    v->addWidget(tradeTable_, 2);

    auto* exportBtn = new QPushButton("Export trades to CSV…");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportTrades);
    auto* eb = new QHBoxLayout;
    eb->addStretch();
    eb->addWidget(exportBtn);
    v->addLayout(eb);
    return w;
}

QWidget* MainWindow::buildBacktestTab() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(16, 16, 16, 16);
    v->setSpacing(12);

    auto* bar = new QHBoxLayout;
    auto* run = new QPushButton("▶  Run backtest");
    run->setMinimumHeight(34);
    connect(run, &QPushButton::clicked, this, &MainWindow::runBacktest);
    btBars_ = ispin(50, 100000, 2000, " bars");
    btVol_ = dspin(0.1, 20, 2.0, 0.5, 2, " %/bar");
    btDrift_ = dspin(-1, 1, 0.01, 0.01, 3, " %/bar");
    btSeed_ = ispin(0, 999999, 42);
    bar->addWidget(run);
    bar->addSpacing(16);
    bar->addWidget(new QLabel("Length"));
    bar->addWidget(btBars_);
    bar->addWidget(new QLabel("Volatility"));
    bar->addWidget(btVol_);
    bar->addWidget(new QLabel("Drift"));
    bar->addWidget(btDrift_);
    bar->addWidget(new QLabel("Seed"));
    bar->addWidget(btSeed_);
    bar->addStretch();
    v->addLayout(bar);

    auto* kg = new QGridLayout;
    kg->setSpacing(10);
    bReturn_ = makeKpi(w, kg, 0, "Strategy return");
    bBuyHold_ = makeKpi(w, kg, 1, "Buy & hold");
    bTrades_ = makeKpi(w, kg, 2, "Trades");
    bWin_ = makeKpi(w, kg, 3, "Win rate");
    bDd_ = makeKpi(w, kg, 4, "Max drawdown");
    bSharpe_ = makeKpi(w, kg, 5, "Sharpe (ann.)");
    v->addLayout(kg);

    equityPlot_ = new PlotWidget;
    equityPlot_->setTitle("Equity curve  (dashed = starting capital)");
    equityPlot_->setPlaceholder("Run a backtest to see results");
    equityPlot_->setValuePrefix("$");
    equityPlot_->setLineColor(QColor(0x9B, 0x7B, 0xFF));
    v->addWidget(equityPlot_, 1);

    auto* note = new QLabel("Synthetic data: results show how the strategy behaves on a random walk, "
                            "not a prediction of real-market performance.");
    note->setStyleSheet("color:gray;");
    note->setWordWrap(true);
    v->addWidget(note);
    return w;
}

StrategyParams MainWindow::currentParams() const {
    StrategyParams p;
    p.kind = static_cast<StrategyKind>(strategyBox_->currentIndex());
    p.position_size = posSize_->value();
    p.stop_loss_pct = stopLoss_->value();
    p.take_profit_pct = takeProfit_->value();
    p.short_period = shortPeriod_->value();
    p.long_period = longPeriod_->value();
    p.momentum_threshold = momThreshold_->value();
    p.lookback_period = (p.kind == StrategyKind::MeanReversion ? lookback_ : window_)->value();
    p.entry_threshold_pct = entryPct_->value();
    p.exit_threshold_pct = exitPct_->value();
    return p;
}

void MainWindow::setRunning(bool running) {
    running_ = running;
    runButton_->setText(running ? "■  Stop" : "▶  Start");
    statusBar()->showMessage(running ? "Simulation running…" : "Stopped.");
}

void MainWindow::toggleRun() {
    if (running_) {
        emit stopSim();
        setRunning(false);
        return;
    }
    StrategyParams p = currentParams();
    if (p.kind == StrategyKind::Momentum && p.short_period >= p.long_period) {
        QMessageBox::warning(this, "Invalid settings", "Short MA must be smaller than Long MA.");
        return;
    }
    prices_.clear();
    markers_.clear();
    firstIndex_ = 0;
    tradeTable_->setRowCount(0);
    pricePlot_->clear();
    setRunning(true);
    emit startSim(p, liveVol_->value(), tickMs_->value(), startPrice_->value());
}

void MainWindow::onTick(int index, double price) {
    if (!running_) return;
    prices_.append(price);
    if (prices_.size() > kMaxPoints) {
        prices_.removeFirst();
        ++firstIndex_;
    }
    // Markers are stored with global indices; shift to the visible window.
    QVector<PlotWidget::Marker> visible;
    for (const auto& m : markers_) {
        int local = m.index - firstIndex_;
        if (local >= 0) visible.append({local, m.value, m.buy});
    }
    pricePlot_->setSeries(prices_, visible);
    kPrice_->setText(QString("$%1").arg(price, 0, 'f', 2));
    Q_UNUSED(index);
}

void MainWindow::onTrade(bool buy, int index, double price, double qty, double pnlPct, double pnlAbs, QString reason) {
    markers_.append({index, price, buy});
    int row = 0;
    tradeTable_->insertRow(row);
    auto item = [](const QString& s, Qt::Alignment a = Qt::AlignLeft | Qt::AlignVCenter) {
        auto* it = new QTableWidgetItem(s);
        it->setTextAlignment(a);
        return it;
    };
    const Qt::Alignment right = Qt::AlignRight | Qt::AlignVCenter;
    auto* side = item(buy ? "BUY" : "SELL");
    side->setForeground(QColor(buy ? kGreen : kRed));
    tradeTable_->setItem(row, 0, item(QDateTime::currentDateTime().toString("hh:mm:ss")));
    tradeTable_->setItem(row, 1, side);
    tradeTable_->setItem(row, 2, item(QString("$%1").arg(price, 0, 'f', 2), right));
    tradeTable_->setItem(row, 3, item(QString::number(qty, 'f', 0), right));
    auto* pnl = item(buy ? "—" : QString("%1$%2 (%3%)").arg(pnlAbs >= 0 ? "+" : "-").arg(std::abs(pnlAbs), 0, 'f', 2).arg(pnlPct, 0, 'f', 2), right);
    if (!buy) pnl->setForeground(QColor(pnlAbs >= 0 ? kGreen : kRed));
    tradeTable_->setItem(row, 4, pnl);
    tradeTable_->setItem(row, 5, item(reason));
}

void MainWindow::onAccount(double equity, double, double position, double realized, int trades, int wins) {
    kEquity_->setText(QString("$%1").arg(equity, 0, 'f', 2));
    kPnl_->setText(QString("%1$%2").arg(realized >= 0 ? "+" : "-").arg(std::abs(realized), 0, 'f', 2));
    colorize(kPnl_, realized);
    kPosition_->setText(position > 0 ? QString("Long %1").arg(position, 0, 'f', 0) : "Flat");
    kTrades_->setText(QString::number(trades));
    kWinRate_->setText(trades ? QString("%1%").arg(100.0 * wins / trades, 0, 'f', 0) : "—");
}

void MainWindow::runBacktest() {
    StrategyParams p = currentParams();
    if (p.kind == StrategyKind::Momentum && p.short_period >= p.long_period) {
        QMessageBox::warning(this, "Invalid settings", "Short MA must be smaller than Long MA.");
        return;
    }
    std::vector<double> prices = generate_prices(btBars_->value(), 100.0, btDrift_->value() / 100.0,
                                                 btVol_->value() / 100.0, btSeed_->value());
    StrategyHandle h = make_strategy(p);
    Backtester bt;
    BacktestResult r = bt.run(prices, h.name, h.generate, h.reset);

    auto pct = [](QLabel* l, double v) {
        l->setText(QString("%1%2%").arg(v >= 0 ? "+" : "").arg(v, 0, 'f', 2));
        colorize(l, v);
    };
    pct(bReturn_, r.total_return_pct);
    pct(bBuyHold_, (prices.back() / prices.front() - 1.0) * 100.0);
    bTrades_->setText(QString::number(r.num_trades));
    bWin_->setText(r.num_trades ? QString("%1%").arg(r.win_rate_pct, 0, 'f', 0) : "—");
    bDd_->setText(QString("%1%").arg(r.max_drawdown_pct, 0, 'f', 2));
    bSharpe_->setText(QString::number(r.sharpe_approx, 'f', 2));

    QVector<double> eq(r.equity_curve.begin(), r.equity_curve.end());
    equityPlot_->setSeries(eq, {}, SimWorker::kInitialCash);
    statusBar()->showMessage(QString("Backtest finished: %1 trades over %2 bars.").arg(r.num_trades).arg(prices.size()));
}

void MainWindow::exportTrades() {
    if (tradeTable_->rowCount() == 0) {
        QMessageBox::information(this, "Nothing to export", "No trades yet.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Export trades", "trades.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export failed", "Could not write " + path);
        return;
    }
    QTextStream out(&f);
    out << "time,side,price,qty,pnl,reason\n";
    for (int r = tradeTable_->rowCount() - 1; r >= 0; --r) {  // oldest first
        QStringList cols;
        for (int c = 0; c < 6; ++c) {
            QString s = tradeTable_->item(r, c)->text();
            s.replace('"', "\"\"");
            cols << '"' + s + '"';
        }
        out << cols.join(',') << '\n';
    }
    statusBar()->showMessage("Exported to " + path);
}
