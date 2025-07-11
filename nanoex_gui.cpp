#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>
#include <QGridLayout>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QFont>
#include <QFontMetrics>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QSettings>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QRegularExpression>
#include <QColor>
#include <QPalette>
#include "src/chart_widget.h"

// ============================================================================
// HFT System Runner Thread
// ============================================================================

class HFTRunner : public QThread {
    Q_OBJECT

public:
    HFTRunner(QObject* parent = nullptr) : QThread(parent), process_(nullptr) {}
    
    void runHFT() {
        if (process_) {
            process_->terminate();
            process_->waitForFinished();
        }
        
        process_ = new QProcess(this);
        
        // Connect signals
        connect(process_, &QProcess::readyReadStandardOutput, 
                this, &HFTRunner::onOutputReady);
        connect(process_, &QProcess::readyReadStandardError, 
                this, &HFTRunner::onErrorReady);
        connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &HFTRunner::onProcessFinished);
        
        // Start the HFT system
        QString program = "./nanoex";
        QStringList arguments;
        
        process_->start(program, arguments);
        
        if (!process_->waitForStarted()) {
            emit error("Failed to start HFT system: " + process_->errorString());
        }
    }
    
    void stopHFT() {
        if (process_) {
            process_->terminate();
            process_->waitForFinished(5000); // Wait up to 5 seconds
            if (process_->state() != QProcess::NotRunning) {
                process_->kill();
            }
        }
    }
    
    bool isRunning() const {
        return process_ && process_->state() == QProcess::Running;
    }

signals:
    void outputReceived(const QString& text);
    void error(const QString& text);
    void processFinished(int exitCode);

private slots:
    void onOutputReady() {
        if (process_) {
            QString output = QString::fromUtf8(process_->readAllStandardOutput());
            emit outputReceived(output);
        }
    }
    
    void onErrorReady() {
        if (process_) {
            QString error = QString::fromUtf8(process_->readAllStandardError());
            emit outputReceived("ERROR: " + error);
        }
    }
    
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        emit processFinished(exitCode);
    }

private:
    QProcess* process_;
};

// ============================================================================
// Performance Monitor Widget
// ============================================================================

class PerformanceWidget : public QWidget {
    Q_OBJECT

public:
    PerformanceWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
    }
    
    void updateStats(const QString& stats) {
        // Parse and update performance statistics
        statsText_->setPlainText(stats);
        
        // Extract key metrics for display
        updateMetrics(stats);
    }

private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // Title
        QLabel* title = new QLabel("Performance Statistics");
        title->setFont(QFont("Arial", 14, QFont::Bold));
        layout->addWidget(title);
        
        // Stats display
        statsText_ = new QTextEdit();
        statsText_->setFont(QFont("Courier", 10));
        statsText_->setReadOnly(true);
        statsText_->setMaximumHeight(200);
        layout->addWidget(statsText_);
        
        // Key metrics grid
        QGroupBox* metricsGroup = new QGroupBox("Key Metrics");
        QGridLayout* metricsLayout = new QGridLayout(metricsGroup);
        
        // Orders per second
        ordersPerSecLabel_ = new QLabel("0");
        ordersPerSecLabel_->setFont(QFont("Arial", 12, QFont::Bold));
        metricsLayout->addWidget(new QLabel("Orders/sec:"), 0, 0);
        metricsLayout->addWidget(ordersPerSecLabel_, 0, 1);
        
        // Latency
        latencyLabel_ = new QLabel("0 ns");
        latencyLabel_->setFont(QFont("Arial", 12, QFont::Bold));
        metricsLayout->addWidget(new QLabel("Avg Latency:"), 0, 2);
        metricsLayout->addWidget(latencyLabel_, 0, 3);
        
        // Trades
        tradesLabel_ = new QLabel("0");
        tradesLabel_->setFont(QFont("Arial", 12, QFont::Bold));
        metricsLayout->addWidget(new QLabel("Trades:"), 1, 0);
        metricsLayout->addWidget(tradesLabel_, 1, 1);
        
        // Spread
        spreadLabel_ = new QLabel("$0.00");
        spreadLabel_->setFont(QFont("Arial", 12, QFont::Bold));
        metricsLayout->addWidget(new QLabel("Spread:"), 1, 2);
        metricsLayout->addWidget(spreadLabel_, 1, 3);
        
        layout->addWidget(metricsGroup);
    }
    
    void updateMetrics(const QString& stats) {
        // Simple parsing of key metrics from the stats text
        // In a real implementation, you'd use regex or structured parsing
        
        // Extract orders per second
        QRegularExpression ordersRegex("Orders/sec:\\s*([0-9.]+)");
        QRegularExpressionMatch ordersMatch = ordersRegex.match(stats);
        if (ordersMatch.hasMatch()) {
            ordersPerSecLabel_->setText(ordersMatch.captured(1));
        }
        
        // Extract latency
        QRegularExpression latencyRegex("Avg latency:\\s*([0-9.]+)\\s*ns");
        QRegularExpressionMatch latencyMatch = latencyRegex.match(stats);
        if (latencyMatch.hasMatch()) {
            latencyLabel_->setText(latencyMatch.captured(1) + " ns");
        }
        
        // Extract trades
        QRegularExpression tradesRegex("Trades matched:\\s*([0-9]+)");
        QRegularExpressionMatch tradesMatch = tradesRegex.match(stats);
        if (tradesMatch.hasMatch()) {
            tradesLabel_->setText(tradesMatch.captured(1));
        }
        
        // Extract spread
        QRegularExpression spreadRegex("Spread:\\s*\\$([0-9.]+)");
        QRegularExpressionMatch spreadMatch = spreadRegex.match(stats);
        if (spreadMatch.hasMatch()) {
            spreadLabel_->setText("$" + spreadMatch.captured(1));
        }
    }

private:
    QTextEdit* statsText_;
    QLabel* ordersPerSecLabel_;
    QLabel* latencyLabel_;
    QLabel* tradesLabel_;
    QLabel* spreadLabel_;
};

// ============================================================================
// Strategy Performance Table
// ============================================================================

class StrategyTable : public QTableWidget {
    Q_OBJECT

public:
    StrategyTable(QWidget* parent = nullptr) : QTableWidget(parent) {
        setupTable();
    }
    
    void updateStrategyData(const QString& strategyName, const QString& stats) {
        // Find or create row for strategy
        int row = findStrategyRow(strategyName);
        if (row == -1) {
            row = rowCount();
            insertRow(row);
            setItem(row, 0, new QTableWidgetItem(strategyName));
        }
        
        // Parse and update strategy statistics
        updateStrategyRow(row, stats);
    }

private:
    void setupTable() {
        setColumnCount(6);
        QStringList headers = {"Strategy", "Signals", "Orders", "Rejected", "Latency (ns)", "PnL"};
        setHorizontalHeaderLabels(headers);
        
        // Set column widths
        horizontalHeader()->setStretchLastSection(true);
        horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        
        // Enable sorting
        setSortingEnabled(true);
    }
    
    int findStrategyRow(const QString& strategyName) {
        for (int i = 0; i < rowCount(); ++i) {
            if (item(i, 0) && item(i, 0)->text() == strategyName) {
                return i;
            }
        }
        return -1;
    }
    
    void updateStrategyRow(int row, const QString& stats) {
        // Parse strategy statistics (simplified parsing)
        QRegularExpression signalsRegex("Signals generated:\\s*([0-9]+)");
        QRegularExpression ordersRegex("Orders sent:\\s*([0-9]+)");
        QRegularExpression rejectedRegex("Orders rejected:\\s*([0-9]+)");
        QRegularExpression latencyRegex("Avg processing time:\\s*([0-9.]+)\\s*ns");
        QRegularExpression pnlRegex("Current PnL:\\s*\\$([0-9.-]+)");
        
        QRegularExpressionMatch signalsMatch = signalsRegex.match(stats);
        if (signalsMatch.hasMatch()) {
            setItem(row, 1, new QTableWidgetItem(signalsMatch.captured(1)));
        }
        
        QRegularExpressionMatch ordersMatch = ordersRegex.match(stats);
        if (ordersMatch.hasMatch()) {
            setItem(row, 2, new QTableWidgetItem(ordersMatch.captured(1)));
        }
        
        QRegularExpressionMatch rejectedMatch = rejectedRegex.match(stats);
        if (rejectedMatch.hasMatch()) {
            setItem(row, 3, new QTableWidgetItem(rejectedMatch.captured(1)));
        }
        
        QRegularExpressionMatch latencyMatch = latencyRegex.match(stats);
        if (latencyMatch.hasMatch()) {
            setItem(row, 4, new QTableWidgetItem(latencyMatch.captured(1)));
        }
        
        QRegularExpressionMatch pnlMatch = pnlRegex.match(stats);
        if (pnlMatch.hasMatch()) {
            setItem(row, 5, new QTableWidgetItem("$" + pnlMatch.captured(1)));
        }
    }
};

// ============================================================================
// Strategy Visualization Widget
// ============================================================================

class StrategyVisualizationWidget : public QWidget {
    Q_OBJECT

public:
    StrategyVisualizationWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
    }
    
    void addPricePoint(double price, const QDateTime& timestamp) {
        chartWidget_->addPricePoint(price, timestamp);
    }
    
    void addSignalPoint(double price, bool isBuy, const QDateTime& timestamp) {
        chartWidget_->addSignalPoint(price, isBuy, timestamp);
    }
    
    void updateIndicators(double rsi, double momentum, double macd) {
        chartWidget_->updateIndicators(rsi, momentum, macd);
    }

private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // Title
        QLabel* title = new QLabel("Momentum Strategy - Real-time Price Chart");
        title->setFont(QFont("Arial", 16, QFont::Bold));
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("QLabel { background-color: #e0e0e0; padding: 10px; border: 2px solid #ccc; }");
        layout->addWidget(title);
        
        // Chart widget
        chartWidget_ = new ChartWidget();
        chartWidget_->setMinimumSize(800, 500);
        layout->addWidget(chartWidget_);
        
        // Instructions
        QLabel* instructions = new QLabel("💡 Hover over the chart to see price and time details");
        instructions->setFont(QFont("Arial", 10));
        instructions->setAlignment(Qt::AlignCenter);
        instructions->setStyleSheet("QLabel { color: #666; padding: 5px; }");
        layout->addWidget(instructions);
    }

private:
    ChartWidget* chartWidget_;
};

// ============================================================================
// Strategy Configuration Widget
// ============================================================================

class StrategyConfigWidget : public QWidget {
    Q_OBJECT

public:
    StrategyConfigWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
    }
    
    void updateConfig(const QString& configText) {
        configText_->setPlainText(configText);
    }

private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        QLabel* title = new QLabel("Strategy Configuration");
        title->setFont(QFont("Arial", 14, QFont::Bold));
        layout->addWidget(title);
        
        configText_ = new QTextEdit();
        configText_->setFont(QFont("Courier", 10));
        configText_->setReadOnly(true);
        configText_->setMaximumHeight(200);
        layout->addWidget(configText_);
    }

private:
    QTextEdit* configText_;
};

// ============================================================================
// Signal Monitor Widget
// ============================================================================

class SignalMonitorWidget : public QWidget {
    Q_OBJECT

public:
    SignalMonitorWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
    }
    
    void addSignal(const QString& signalText) {
        signalsText_->append(QDateTime::currentDateTime().toString("hh:mm:ss") + " " + signalText);
        
        // Keep only last 100 signals
        QStringList lines = signalsText_->toPlainText().split('\n');
        if (lines.size() > 100) {
            lines = lines.mid(lines.size() - 100);
            signalsText_->setPlainText(lines.join('\n'));
        }
        
        // Auto-scroll to bottom
        QScrollBar* scrollbar = signalsText_->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
    }

private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        QLabel* title = new QLabel("Strategy Signals");
        title->setFont(QFont("Arial", 14, QFont::Bold));
        layout->addWidget(title);
        
        signalsText_ = new QTextEdit();
        signalsText_->setFont(QFont("Courier", 9));
        signalsText_->setReadOnly(true);
        layout->addWidget(signalsText_);
    }

private:
    QTextEdit* signalsText_;
};

// ============================================================================
// Main Window
// ============================================================================

class NanoEXMainWindow : public QMainWindow {
    Q_OBJECT

public:
    NanoEXMainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupMenuBar();
        setupStatusBar();
        
        // Initialize HFT runner
        hftRunner_ = new HFTRunner(this);
        connect(hftRunner_, &HFTRunner::outputReceived, 
                this, &NanoEXMainWindow::onOutputReceived);
        connect(hftRunner_, &HFTRunner::error, 
                this, &NanoEXMainWindow::onError);
        connect(hftRunner_, &HFTRunner::processFinished, 
                this, &NanoEXMainWindow::onProcessFinished);
        
        // Setup timer for periodic updates
        updateTimer_ = new QTimer(this);
        connect(updateTimer_, &QTimer::timeout, this, &NanoEXMainWindow::onUpdateTimer);
        
        // Load settings
        loadSettings();
    }
    
    ~NanoEXMainWindow() {
        saveSettings();
        if (hftRunner_->isRunning()) {
            hftRunner_->stopHFT();
        }
    }

private slots:
    void onStartButtonClicked() {
        if (!hftRunner_->isRunning()) {
            startButton_->setText("Stop HFT System");
            startButton_->setStyleSheet("QPushButton { background-color: #ff4444; color: white; }");
            
            // Clear previous output
            outputText_->clear();
            strategyTable_->setRowCount(0);
            
            // Start HFT system
            hftRunner_->runHFT();
            
            // Start update timer
            updateTimer_->start(1000); // Update every second
            
            statusBar()->showMessage("HFT System started");
        } else {
            onStopButtonClicked();
        }
    }
    
    void onStopButtonClicked() {
        if (hftRunner_->isRunning()) {
            hftRunner_->stopHFT();
            startButton_->setText("Start HFT System");
            startButton_->setStyleSheet("QPushButton { background-color: #44ff44; color: white; }");
            
            updateTimer_->stop();
            statusBar()->showMessage("HFT System stopped");
        }
    }
    
    void onClearButtonClicked() {
        outputText_->clear();
    }
    
    void onOutputReceived(const QString& text) {
        outputText_->append(text);
        
        // Auto-scroll to bottom
        QScrollBar* scrollbar = outputText_->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
        
        // Parse for performance updates
        if (text.contains("=== NanoEX Performance Stats ===")) {
            // Extract performance stats
            QString stats = extractPerformanceStats();
            performanceWidget_->updateStats(stats);
        }
        
        // Parse for strategy updates
        if (text.contains("Strategy '") && text.contains("':")) {
            parseStrategyUpdate(text);
        }
        
        // Parse for strategy signals and update visualization
        parseStrategySignals(text);
    }
    
    void onError(const QString& error) {
        outputText_->append("ERROR: " + error);
        QMessageBox::warning(this, "HFT System Error", error);
    }
    
    void onProcessFinished(int exitCode) {
        startButton_->setText("Start HFT System");
        startButton_->setStyleSheet("QPushButton { background-color: #44ff44; color: white; }");
        updateTimer_->stop();
        
        QString message = QString("HFT System finished with exit code: %1").arg(exitCode);
        statusBar()->showMessage(message);
        
        if (exitCode != 0) {
            QMessageBox::information(this, "HFT System", 
                                   "HFT System completed with warnings or errors. Check output for details.");
        }
    }
    
    void onUpdateTimer() {
        // Periodic updates can be added here
        // For now, just update the status
        if (hftRunner_->isRunning()) {
            statusBar()->showMessage("HFT System running...");
        }
    }
    
    void onCompileAction() {
        QProcess* compileProcess = new QProcess(this);
        compileProcess->setWorkingDirectory(QDir::currentPath());
        
        connect(compileProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, compileProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitCode == 0) {
                        QMessageBox::information(this, "Compilation", "HFT System compiled successfully!");
                    } else {
                        QString error = QString::fromUtf8(compileProcess->readAllStandardError());
                        QMessageBox::critical(this, "Compilation Error", 
                                            "Compilation failed:\n" + error);
                    }
                    compileProcess->deleteLater();
                });
        
        QStringList arguments = {"-std=c++17", "-O2", "-pthread", "nanoex.cpp", "-o", "nanoex"};
        compileProcess->start("g++", arguments);
    }
    
    void onSaveLogAction() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Log File", 
                                                      "", "Text Files (*.txt);;All Files (*)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << outputText_->toPlainText();
                file.close();
                statusBar()->showMessage("Log saved to " + fileName);
            } else {
                QMessageBox::critical(this, "Error", "Failed to save log file");
            }
        }
    }

private:
    void setupUI() {
        setWindowTitle("NanoEX HFT System - Momentum Strategy Visualization");
        setMinimumSize(1400, 900);
        
        // Central widget
        QWidget* centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        
        // Main layout
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
        
        // Control panel
        QGroupBox* controlGroup = new QGroupBox("System Control");
        QHBoxLayout* controlLayout = new QHBoxLayout(controlGroup);
        
        startButton_ = new QPushButton("Start HFT System");
        startButton_->setStyleSheet("QPushButton { background-color: #44ff44; color: white; font-weight: bold; padding: 10px; }");
        startButton_->setMinimumHeight(40);
        connect(startButton_, &QPushButton::clicked, this, &NanoEXMainWindow::onStartButtonClicked);
        
        clearButton_ = new QPushButton("Clear Output");
        clearButton_->setMinimumHeight(40);
        connect(clearButton_, &QPushButton::clicked, this, &NanoEXMainWindow::onClearButtonClicked);
        
        controlLayout->addWidget(startButton_);
        controlLayout->addWidget(clearButton_);
        controlLayout->addStretch();
        
        mainLayout->addWidget(controlGroup);
        
        // Create tab widget for different views
        QTabWidget* tabWidget = new QTabWidget();
        
        // Strategy Chart tab
        QWidget* strategyTab = new QWidget();
        QVBoxLayout* strategyLayout = new QVBoxLayout(strategyTab);
        
        strategyChart_ = new StrategyVisualizationWidget();
        strategyLayout->addWidget(strategyChart_);
        
        tabWidget->addTab(strategyTab, "Strategy Chart");
        
        // Signals tab
        QWidget* signalsTab = new QWidget();
        QVBoxLayout* signalsLayout = new QVBoxLayout(signalsTab);
        
        signalMonitor_ = new SignalMonitorWidget();
        signalsLayout->addWidget(signalMonitor_);
        
        tabWidget->addTab(signalsTab, "Strategy Signals");
        
        // Configuration tab
        QWidget* configTab = new QWidget();
        QVBoxLayout* configLayout = new QVBoxLayout(configTab);
        
        strategyConfig_ = new StrategyConfigWidget();
        configLayout->addWidget(strategyConfig_);
        
        tabWidget->addTab(configTab, "Strategy Config");
        
        // Performance tab
        QWidget* performanceTab = new QWidget();
        QVBoxLayout* performanceLayout = new QVBoxLayout(performanceTab);
        
        performanceWidget_ = new PerformanceWidget();
        performanceLayout->addWidget(performanceWidget_);
        
        strategyTable_ = new StrategyTable();
        performanceLayout->addWidget(strategyTable_);
        
        tabWidget->addTab(performanceTab, "Performance");
        
        // Output tab
        QWidget* outputTab = new QWidget();
        QVBoxLayout* outputLayout = new QVBoxLayout(outputTab);
        
        outputText_ = new QTextEdit();
        outputText_->setFont(QFont("Courier", 9));
        outputText_->setReadOnly(true);
        outputLayout->addWidget(outputText_);
        
        tabWidget->addTab(outputTab, "System Output");
        
        mainLayout->addWidget(tabWidget);
    }
    
    void setupMenuBar() {
        QMenuBar* menuBar = this->menuBar();
        
        // File menu
        QMenu* fileMenu = menuBar->addMenu("&File");
        
        QAction* compileAction = new QAction("&Compile HFT System", this);
        compileAction->setShortcut(QKeySequence("Ctrl+B"));
        connect(compileAction, &QAction::triggered, this, &NanoEXMainWindow::onCompileAction);
        fileMenu->addAction(compileAction);
        
        QAction* saveLogAction = new QAction("&Save Log", this);
        saveLogAction->setShortcut(QKeySequence("Ctrl+S"));
        connect(saveLogAction, &QAction::triggered, this, &NanoEXMainWindow::onSaveLogAction);
        fileMenu->addAction(saveLogAction);
        
        fileMenu->addSeparator();
        
        QAction* exitAction = new QAction("E&xit", this);
        exitAction->setShortcut(QKeySequence("Ctrl+Q"));
        connect(exitAction, &QAction::triggered, this, &QWidget::close);
        fileMenu->addAction(exitAction);
        
        // Help menu
        QMenu* helpMenu = menuBar->addMenu("&Help");
        
        QAction* aboutAction = new QAction("&About", this);
        connect(aboutAction, &QAction::triggered, [this]() {
            QMessageBox::about(this, "About NanoEX", 
                              "NanoEX High-Frequency Trading System\n\n"
                              "A high-performance C++ trading engine demonstrating\n"
                              "advanced multi-threading and real-time processing.\n\n"
                              "Version 1.0");
        });
        helpMenu->addAction(aboutAction);
    }
    
    void setupStatusBar() {
        statusBar()->showMessage("Ready");
    }
    
    QString extractPerformanceStats() {
        QString text = outputText_->toPlainText();
        int startPos = text.lastIndexOf("=== NanoEX Performance Stats ===");
        if (startPos != -1) {
            int endPos = text.indexOf("===", startPos + 1);
            if (endPos != -1) {
                return text.mid(startPos, endPos - startPos + 3);
            }
        }
        return "";
    }
    
    void parseStrategyUpdate(const QString& text) {
        // Simple parsing of strategy updates
        // In a real implementation, you'd use more sophisticated parsing
        
        QRegularExpression strategyRegex("Strategy '([^']+)':");
        QRegularExpressionMatch strategyMatch = strategyRegex.match(text);
        if (strategyMatch.hasMatch()) {
            QString strategyName = strategyMatch.captured(1);
            strategyTable_->updateStrategyData(strategyName, text);
        }
    }
    
    void parseStrategySignals(const QString& text) {
        // Parse BUY signals
        if (text.contains("🟢 BUY Signal:")) {
            signalMonitor_->addSignal(text);
            
            // Extract price from signal
            QRegularExpression priceRegex("@ ([0-9.]+)");
            QRegularExpressionMatch priceMatch = priceRegex.match(text);
            if (priceMatch.hasMatch()) {
                double price = priceMatch.captured(1).toDouble();
                strategyChart_->addSignalPoint(price, true, QDateTime::currentDateTime());
            }
        }
        
        // Parse SELL signals
        if (text.contains("🔴 SELL Signal:")) {
            signalMonitor_->addSignal(text);
            
            // Extract price from signal
            QRegularExpression priceRegex("@ ([0-9.]+)");
            QRegularExpressionMatch priceMatch = priceRegex.match(text);
            if (priceMatch.hasMatch()) {
                double price = priceMatch.captured(1).toDouble();
                strategyChart_->addSignalPoint(price, false, QDateTime::currentDateTime());
            }
        }
        
        // Parse strategy configuration
        if (text.contains("=== Momentum Strategy Configuration ===")) {
            // Extract configuration text
            QStringList lines = text.split('\n');
            QString configText;
            bool inConfig = false;
            for (const QString& line : lines) {
                if (line.contains("=== Momentum Strategy Configuration ===")) {
                    inConfig = true;
                    continue;
                }
                if (line.contains("=====================================")) {
                    inConfig = false;
                    break;
                }
                if (inConfig) {
                    configText += line + "\n";
                }
            }
            strategyConfig_->updateConfig(configText);
        }
        
        // Parse indicators from signal reasons
        if (text.contains("Momentum:") && text.contains("RSI:") && text.contains("MACD:")) {
            QRegularExpression momentumRegex("Momentum: ([0-9.-]+)");
            QRegularExpression rsiRegex("RSI: ([0-9.]+)");
            QRegularExpression macdRegex("MACD: ([A-Za-z]+)");
            
            QRegularExpressionMatch momentumMatch = momentumRegex.match(text);
            QRegularExpressionMatch rsiMatch = rsiRegex.match(text);
            QRegularExpressionMatch macdMatch = macdRegex.match(text);
            
            if (momentumMatch.hasMatch() && rsiMatch.hasMatch()) {
                double momentum = momentumMatch.captured(1).toDouble();
                double rsi = rsiMatch.captured(1).toDouble();
                double macd = macdMatch.hasMatch() ? (macdMatch.captured(1) == "Bullish" ? 1.0 : -1.0) : 0.0;
                
                strategyChart_->updateIndicators(rsi, momentum, macd);
            }
        }
        
        // Parse price data from orders
        if (text.contains("📊 Order:") && text.contains("@")) {
            QRegularExpression priceRegex("@ ([0-9.]+)");
            QRegularExpressionMatch priceMatch = priceRegex.match(text);
            if (priceMatch.hasMatch()) {
                double price = priceMatch.captured(1).toDouble();
                strategyChart_->addPricePoint(price, QDateTime::currentDateTime());
            }
        }
    }
    
    void loadSettings() {
        QSettings settings("NanoEX", "HFTSystem");
        restoreGeometry(settings.value("geometry").toByteArray());
        restoreState(settings.value("windowState").toByteArray());
    }
    
    void saveSettings() {
        QSettings settings("NanoEX", "HFTSystem");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
    }

private:
    QPushButton* startButton_;
    QPushButton* clearButton_;
    QTextEdit* outputText_;
    PerformanceWidget* performanceWidget_;
    StrategyTable* strategyTable_;
    StrategyVisualizationWidget* strategyChart_;
    SignalMonitorWidget* signalMonitor_;
    StrategyConfigWidget* strategyConfig_;
    HFTRunner* hftRunner_;
    QTimer* updateTimer_;
};

// ============================================================================
// Main Function
// ============================================================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("NanoEX HFT System");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("NanoEX");
    
    // Create and show main window
    NanoEXMainWindow window;
    window.show();
    
    return app.exec();
}

#include "nanoex_gui.moc" 