#pragma once
#include <QVector>
#include <QWidget>

// Grafico live RX/TX (bit/s) con curve morbide, riempimento sfumato e leggero glow.
class Sparkline : public QWidget {
    Q_OBJECT
public:
    explicit Sparkline(QWidget* parent = nullptr, int capacity = 60);

    void push(double rxBps, double txBps);
    void clear();

    QSize minimumSizeHint() const override { return {200, 96}; }
    QSize sizeHint() const override { return {400, 110}; }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void drawSeries(QPainter& p, const QRectF& r, const QVector<double>& v, const QColor& color) const;

    int m_cap;
    QVector<double> m_rx, m_tx;
    double m_peak = 1e6;
};
