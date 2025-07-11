#include "chart_widget.h"
#include <QApplication>
#include <QFontMetrics>
#include <QDebug>
#include <QPainterPath>

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(600, 400);
    setMouseTracking(true);
    
    // Initialize time range
    minTime_ = QDateTime::currentDateTime();
    maxTime_ = minTime_.addSecs(60); // 1 minute window
    
    // Set up timer for auto-refresh
    QTimer* refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, [this]() { update(); });
    refreshTimer->start(100); // Update 10 times per second
}

void ChartWidget::addPricePoint(double price, const QDateTime& timestamp) {
    DataPoint point(timestamp, price);
    point.rsi = currentRSI_;
    point.momentum = currentMomentum_;
    point.macd = currentMACD_;
    
    dataPoints_.append(point);
    
    // Keep only last 200 points for performance
    if (dataPoints_.size() > 200) {
        dataPoints_.removeFirst();
    }
    
    updateScales();
    update();
}

void ChartWidget::addSignalPoint(double price, bool isBuy, const QDateTime& timestamp) {
    DataPoint signal(timestamp, price);
    signal.isSignal = true;
    signal.isBuySignal = isBuy;
    signal.rsi = currentRSI_;
    signal.momentum = currentMomentum_;
    signal.macd = currentMACD_;
    
    signals_.append(signal);
    
    // Keep only last 50 signals
    if (signals_.size() > 50) {
        signals_.removeFirst();
    }
    
    update();
}

void ChartWidget::updateIndicators(double rsi, double momentum, double macd) {
    currentRSI_ = rsi;
    currentMomentum_ = momentum;
    currentMACD_ = macd;
    update();
}

void ChartWidget::clearData() {
    dataPoints_.clear();
    signals_.clear();
    update();
}

void ChartWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill background
    painter.fillRect(rect(), backgroundColor_);
    
    if (dataPoints_.isEmpty()) {
        // Draw placeholder text
        painter.setPen(Qt::gray);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(rect(), Qt::AlignCenter, "Waiting for price data...\nStart the HFT system to see the chart");
        return;
    }
    
    // Draw chart elements
    drawGrid(painter);
    drawPriceChart(painter);
    drawSignals(painter);
    drawIndicators(painter);
}

void ChartWidget::drawGrid(QPainter& painter) {
    painter.setPen(QPen(gridColor_, 1, Qt::DotLine));
    
    int chartWidth = width() - marginLeft_ - marginRight_;
    int chartHeight = height() - marginTop_ - marginBottom_;
    
    // Vertical grid lines (time)
    for (int i = 0; i <= 10; ++i) {
        int x = marginLeft_ + (i * chartWidth) / 10;
        painter.drawLine(x, marginTop_, x, height() - marginBottom_);
    }
    
    // Horizontal grid lines (price)
    for (int i = 0; i <= 5; ++i) {
        int y = marginTop_ + (i * chartHeight) / 5;
        painter.drawLine(marginLeft_, y, width() - marginRight_, y);
    }
    
    // Draw axis labels
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    
    // Price labels
    for (int i = 0; i <= 5; ++i) {
        int y = marginTop_ + (i * chartHeight) / 5;
        double price = maxPrice_ - (i * (maxPrice_ - minPrice_)) / 5;
        painter.drawText(5, y + 4, QString("$%1").arg(price, 0, 'f', 2));
    }
    
    // Time labels
    for (int i = 0; i <= 5; ++i) {
        int x = marginLeft_ + (i * chartWidth) / 5;
        qint64 timeOffset = (i * (maxTime_.toMSecsSinceEpoch() - minTime_.toMSecsSinceEpoch())) / 5;
        QDateTime labelTime = minTime_.addMSecs(timeOffset);
        painter.drawText(x - 20, height() - marginBottom_ + 15, labelTime.toString("hh:mm:ss"));
    }
}

void ChartWidget::drawPriceChart(QPainter& painter) {
    if (dataPoints_.size() < 2) return;
    
    painter.setPen(QPen(priceColor_, 2));
    
    QPainterPath path;
    bool first = true;
    
    for (const DataPoint& point : dataPoints_) {
        QPointF screenPoint = dataToScreen(point.timestamp, point.price);
        
        if (first) {
            path.moveTo(screenPoint);
            first = false;
        } else {
            path.lineTo(screenPoint);
        }
    }
    
    painter.drawPath(path);
}

void ChartWidget::drawSignals(QPainter& painter) {
    for (const DataPoint& signal : signals_) {
        QPointF screenPoint = dataToScreen(signal.timestamp, signal.price);
        
        // Draw signal marker
        QColor signalColor = signal.isBuySignal ? buySignalColor_ : sellSignalColor_;
        painter.setPen(QPen(signalColor, 3));
        painter.setBrush(signalColor);
        
        // Draw circle for signal
        painter.drawEllipse(screenPoint, 6, 6);
        
        // Draw signal type indicator
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        QString signalText = signal.isBuySignal ? "B" : "S";
        painter.drawText(screenPoint.x() - 3, screenPoint.y() + 3, signalText);
    }
}

void ChartWidget::drawIndicators(QPainter& painter) {
    int chartWidth = width() - marginLeft_ - marginRight_;
    int chartHeight = height() - marginTop_ - marginBottom_;
    
    // Draw indicator panel at the top
    QRect indicatorRect(marginLeft_, 5, chartWidth, 30);
    painter.fillRect(indicatorRect, QColor(240, 240, 240));
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    
    // RSI with color coding
    QColor rsiColor = Qt::black;
    if (currentRSI_ < 30) rsiColor = Qt::green;
    else if (currentRSI_ > 70) rsiColor = Qt::red;
    
    painter.setPen(rsiColor);
    painter.drawText(indicatorRect.x() + 10, indicatorRect.y() + 20, 
                    QString("RSI: %1").arg(currentRSI_, 0, 'f', 1));
    
    // Momentum with color coding
    QColor momentumColor = Qt::black;
    if (currentMomentum_ > 0.3) momentumColor = Qt::green;
    else if (currentMomentum_ < -0.3) momentumColor = Qt::red;
    
    painter.setPen(momentumColor);
    painter.drawText(indicatorRect.x() + 120, indicatorRect.y() + 20, 
                    QString("Momentum: %1").arg(currentMomentum_, 0, 'f', 2));
    
    // MACD
    painter.setPen(Qt::black);
    painter.drawText(indicatorRect.x() + 250, indicatorRect.y() + 20, 
                    QString("MACD: %1").arg(currentMACD_, 0, 'f', 2));
    
    // Current price
    if (!dataPoints_.isEmpty()) {
        double currentPrice = dataPoints_.last().price;
        painter.setPen(priceColor_);
        painter.drawText(indicatorRect.x() + 380, indicatorRect.y() + 20, 
                        QString("Price: $%1").arg(currentPrice, 0, 'f', 2));
    }
}

void ChartWidget::updateScales() {
    if (dataPoints_.isEmpty()) return;
    
    // Update price scale
    minPrice_ = dataPoints_.first().price;
    maxPrice_ = dataPoints_.first().price;
    
    for (const DataPoint& point : dataPoints_) {
        minPrice_ = qMin(minPrice_, point.price);
        maxPrice_ = qMax(maxPrice_, point.price);
    }
    
    // Add some padding
    double priceRange = maxPrice_ - minPrice_;
    if (priceRange < 0.1) priceRange = 0.1;
    minPrice_ -= priceRange * 0.1;
    maxPrice_ += priceRange * 0.1;
    
    // Update time scale
    minTime_ = dataPoints_.first().timestamp;
    maxTime_ = dataPoints_.last().timestamp;
    
    // Ensure minimum time range
    if (minTime_.msecsTo(maxTime_) < 10000) { // Less than 10 seconds
        maxTime_ = minTime_.addMSecs(10000);
    }
}

QPointF ChartWidget::dataToScreen(const QDateTime& timestamp, double value, bool isPrice) {
    int chartWidth = width() - marginLeft_ - marginRight_;
    int chartHeight = height() - marginTop_ - marginBottom_;
    
    // Convert time to x coordinate
    qint64 timeRange = minTime_.msecsTo(maxTime_);
    if (timeRange == 0) timeRange = 1;
    qint64 timeOffset = minTime_.msecsTo(timestamp);
    int x = marginLeft_ + (timeOffset * chartWidth) / timeRange;
    
    // Convert price to y coordinate
    double priceRange = maxPrice_ - minPrice_;
    if (priceRange == 0) priceRange = 1;
    double priceOffset = value - minPrice_;
    int y = height() - marginBottom_ - (priceOffset * chartHeight) / priceRange;
    
    return QPointF(x, y);
}

QDateTime ChartWidget::screenToTime(int x) {
    int chartWidth = width() - marginLeft_ - marginRight_;
    qint64 timeRange = minTime_.msecsTo(maxTime_);
    qint64 timeOffset = ((x - marginLeft_) * timeRange) / chartWidth;
    return minTime_.addMSecs(timeOffset);
}

double ChartWidget::screenToPrice(int y) {
    int chartHeight = height() - marginTop_ - marginBottom_;
    double priceRange = maxPrice_ - minPrice_;
    double priceOffset = ((height() - marginBottom_ - y) * priceRange) / chartHeight;
    return minPrice_ + priceOffset;
}

void ChartWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateScales();
}

void ChartWidget::mouseMoveEvent(QMouseEvent* event) {
    if (dataPoints_.isEmpty()) return;
    
    QPoint pos = event->pos();
    if (pos.x() >= marginLeft_ && pos.x() <= width() - marginRight_ &&
        pos.y() >= marginTop_ && pos.y() <= height() - marginBottom_) {
        
        QDateTime time = screenToTime(pos.x());
        double price = screenToPrice(pos.y());
        
        QString tooltip = QString("Time: %1\nPrice: $%2")
                         .arg(time.toString("hh:mm:ss.zzz"))
                         .arg(price, 0, 'f', 2);
        
        QToolTip::showText(event->globalPosition().toPoint(), tooltip, this);
    }
} 