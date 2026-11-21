#include "ui/Sparkline.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>

Sparkline::Sparkline(QWidget* parent, int capacity) : QWidget(parent), m_cap(capacity) {}

void Sparkline::push(double rxBps, double txBps) {
    m_rx.append(rxBps);
    m_tx.append(txBps);
    while (m_rx.size() > m_cap) m_rx.removeFirst();
    while (m_tx.size() > m_cap) m_tx.removeFirst();

    double peak = 1e6;
    for (double v : m_rx) peak = std::max(peak, v);
    for (double v : m_tx) peak = std::max(peak, v);
    m_peak += (peak * 1.2 - m_peak) * 0.35; // asse che si adatta con inerzia
    update();
}

void Sparkline::clear() {
    m_rx.clear();
    m_tx.clear();
    m_peak = 1e6;
    update();
}

void Sparkline::drawSeries(QPainter& p, const QRectF& r, const QVector<double>& v, const QColor& color) const {
    if (v.size() < 2) return;
    const double step = r.width() / (m_cap - 1);
    const int offset = m_cap - v.size();
    auto pt = [&](int i) {
        const double x = r.left() + (offset + i) * step;
        const double y = r.bottom() - std::min(1.0, v[i] / m_peak) * r.height();
        return QPointF(x, y);
    };

    QPainterPath line;
    line.moveTo(pt(0));
    for (int i = 1; i < v.size(); ++i) {
        const QPointF a = pt(i - 1), b = pt(i);
        const double mx = (a.x() + b.x()) / 2.0;
        line.cubicTo(QPointF(mx, a.y()), QPointF(mx, b.y()), b);
    }

    QPainterPath fill = line;
    fill.lineTo(pt(v.size() - 1).x(), r.bottom());
    fill.lineTo(pt(0).x(), r.bottom());
    fill.closeSubpath();

    QLinearGradient g(0, r.top(), 0, r.bottom());
    QColor top = color; top.setAlpha(95);
    QColor bot = color; bot.setAlpha(0);
    g.setColorAt(0.0, top);
    g.setColorAt(1.0, bot);
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawPath(fill);

    p.setBrush(Qt::NoBrush);
    QColor glow = color; glow.setAlpha(45);
    p.setPen(QPen(glow, 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(line);
    p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(line);
}

void Sparkline::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = QRectF(rect()).adjusted(2, 8, -2, -6);

    p.setPen(QPen(QColor(148, 163, 184, 28), 1));
    for (int i = 0; i <= 3; ++i) {
        const double y = r.top() + r.height() * i / 3.0;
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }

    drawSeries(p, r, m_tx, QColor(167, 139, 250));
    drawSeries(p, r, m_rx, QColor(34, 211, 238));
}
