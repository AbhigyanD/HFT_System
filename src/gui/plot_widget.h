#pragma once
#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

// Minimal line chart: one series, optional reference line, optional buy/sell markers.
class PlotWidget : public QWidget {
    Q_OBJECT
public:
    struct Marker { int index; double value; bool buy; };

    explicit PlotWidget(QWidget* parent = nullptr);

    void setTitle(const QString& title) { title_ = title; update(); }
    void setPlaceholder(const QString& text) { placeholder_ = text; update(); }
    void setLineColor(const QColor& c) { lineColor_ = c; update(); }
    void setValuePrefix(const QString& p) { prefix_ = p; update(); }
    void setSeries(const QVector<double>& values, const QVector<Marker>& markers = {}, double reference = qQNaN());
    void clear();

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    QRectF plotRect() const;
    QString title_, placeholder_ = "No data yet", prefix_;
    QColor lineColor_{0x4C, 0x9B, 0xFF};
    QVector<double> values_;
    QVector<Marker> markers_;
    double reference_ = qQNaN();
    double lo_ = 0, hi_ = 1;
    int hover_ = -1;
};
