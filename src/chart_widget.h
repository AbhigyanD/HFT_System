#pragma once

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QDateTime>
#include <QMouseEvent>
#include <QToolTip>

struct DataPoint {
    QDateTime timestamp;
    double price;
    double rsi;
    double momentum;
    double macd;
    bool isSignal;
    bool isBuySignal;
    
    DataPoint() : price(0), rsi(50), momentum(0), macd(0), isSignal(false), isBuySignal(false) {}
    DataPoint(const QDateTime& t, double p) : timestamp(t), price(p), rsi(50), momentum(0), macd(0), isSignal(false), isBuySignal(false) {}
};

class ChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChartWidget(QWidget* parent = nullptr);
    
    void addPricePoint(double price, const QDateTime& timestamp);
    void addSignalPoint(double price, bool isBuy, const QDateTime& timestamp);
    void updateIndicators(double rsi, double momentum, double macd);
    void clearData();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void drawPriceChart(QPainter& painter);
    void drawIndicators(QPainter& painter);
    void drawSignals(QPainter& painter);
    void drawGrid(QPainter& painter);
    void updateScales();
    QPointF dataToScreen(const QDateTime& timestamp, double value, bool isPrice = true);
    QDateTime screenToTime(int x);
    double screenToPrice(int y);
    
    QVector<DataPoint> dataPoints_;
    QVector<DataPoint> signals_;
    
    // Chart dimensions
    int marginLeft_ = 80;
    int marginRight_ = 20;
    int marginTop_ = 40;
    int marginBottom_ = 60;
    
    // Scales
    double minPrice_ = 0;
    double maxPrice_ = 100;
    QDateTime minTime_;
    QDateTime maxTime_;
    
    // Current indicators
    double currentRSI_ = 50;
    double currentMomentum_ = 0;
    double currentMACD_ = 0;
    
    // Mouse tracking
    QPoint lastMousePos_;
    bool showTooltip_ = false;
    
    // Colors
    QColor priceColor_ = QColor(0, 100, 200);
    QColor buySignalColor_ = QColor(0, 200, 0);
    QColor sellSignalColor_ = QColor(200, 0, 0);
    QColor gridColor_ = QColor(200, 200, 200);
    QColor backgroundColor_ = QColor(255, 255, 255);
}; 