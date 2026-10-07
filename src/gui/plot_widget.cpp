#include "plot_widget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

PlotWidget::PlotWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(360, 220);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PlotWidget::setSeries(const QVector<double>& values, const QVector<Marker>& markers, double reference) {
    values_ = values;
    markers_ = markers;
    reference_ = reference;
    if (!values_.isEmpty()) {
        auto [mn, mx] = std::minmax_element(values_.begin(), values_.end());
        lo_ = *mn;
        hi_ = *mx;
        if (!std::isnan(reference_)) { lo_ = std::min(lo_, reference_); hi_ = std::max(hi_, reference_); }
        double pad = std::max((hi_ - lo_) * 0.08, 1e-6);
        lo_ -= pad;
        hi_ += pad;
    }
    update();
}

void PlotWidget::clear() {
    values_.clear();
    markers_.clear();
    reference_ = qQNaN();
    hover_ = -1;
    update();
}

QRectF PlotWidget::plotRect() const { return QRectF(64, 30, width() - 64 - 16, height() - 30 - 24); }

void PlotWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPalette& pal = palette();
    p.fillRect(rect(), pal.color(QPalette::Base));

    p.setPen(pal.color(QPalette::Text));
    QFont f = font();
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRectF(12, 6, width() - 24, 20), Qt::AlignLeft | Qt::AlignVCenter, title_);
    f.setBold(false);
    p.setFont(f);

    QRectF r = plotRect();
    if (values_.size() < 2) {
        p.setPen(pal.color(QPalette::PlaceholderText));
        p.drawText(r, Qt::AlignCenter, placeholder_);
        return;
    }

    QColor grid = pal.color(QPalette::Text);
    grid.setAlpha(40);
    QColor label = pal.color(QPalette::PlaceholderText);
    for (int i = 0; i <= 4; ++i) {
        double y = r.top() + r.height() * i / 4.0;
        p.setPen(QPen(grid, 1));
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        double v = hi_ - (hi_ - lo_) * i / 4.0;
        p.setPen(label);
        p.drawText(QRectF(0, y - 9, r.left() - 6, 18), Qt::AlignRight | Qt::AlignVCenter,
                   prefix_ + QString::number(v, 'f', std::abs(hi_) > 1000 ? 0 : 2));
    }

    auto toPt = [&](int i, double v) {
        double x = r.left() + r.width() * i / double(values_.size() - 1);
        double y = r.bottom() - r.height() * (v - lo_) / (hi_ - lo_);
        return QPointF(x, y);
    };

    if (!std::isnan(reference_)) {
        p.setPen(QPen(label, 1, Qt::DashLine));
        double y = toPt(0, reference_).y();
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }

    QPainterPath path;
    for (int i = 0; i < values_.size(); ++i) {
        QPointF pt = toPt(i, values_[i]);
        i == 0 ? path.moveTo(pt) : path.lineTo(pt);
    }
    p.setPen(QPen(lineColor_, 2));
    p.drawPath(path);

    for (const Marker& m : markers_) {
        if (m.index < 0 || m.index >= values_.size()) continue;
        QPointF pt = toPt(m.index, m.value);
        QColor c = m.buy ? QColor(0x2E, 0xC4, 0x6B) : QColor(0xF0, 0x4D, 0x4D);
        p.setPen(QPen(pal.color(QPalette::Base), 1.5));
        p.setBrush(c);
        QPolygonF tri;
        if (m.buy) tri << pt + QPointF(0, -3) << pt + QPointF(-6, 9) << pt + QPointF(6, 9);
        else       tri << pt + QPointF(0, 3) << pt + QPointF(-6, -9) << pt + QPointF(6, -9);
        p.drawPolygon(tri);
    }

    if (hover_ >= 0 && hover_ < values_.size()) {
        QPointF pt = toPt(hover_, values_[hover_]);
        p.setPen(QPen(label, 1, Qt::DotLine));
        p.drawLine(QPointF(pt.x(), r.top()), QPointF(pt.x(), r.bottom()));
        p.setBrush(lineColor_);
        p.setPen(Qt::NoPen);
        p.drawEllipse(pt, 4, 4);
        QString text = QString("#%1   %2%3").arg(hover_).arg(prefix_).arg(values_[hover_], 0, 'f', 2);
        QFontMetrics fm(font());
        QRectF box(0, 0, fm.horizontalAdvance(text) + 14, fm.height() + 8);
        box.moveTo(std::min(pt.x() + 10, r.right() - box.width()), r.top() + 4);
        p.setBrush(pal.color(QPalette::ToolTipBase));
        p.setPen(QPen(label, 1));
        p.drawRoundedRect(box, 4, 4);
        p.setPen(pal.color(QPalette::ToolTipText));
        p.drawText(box, Qt::AlignCenter, text);
    }
}

void PlotWidget::mouseMoveEvent(QMouseEvent* e) {
    if (values_.size() < 2) return;
    QRectF r = plotRect();
    double t = (e->position().x() - r.left()) / r.width();
    hover_ = std::clamp(int(std::lround(t * (values_.size() - 1))), 0, int(values_.size()) - 1);
    update();
}

void PlotWidget::leaveEvent(QEvent*) {
    hover_ = -1;
    update();
}
