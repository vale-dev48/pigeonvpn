#pragma once
#include <QEasingCurve>
#include <QPainter>
#include <QPropertyAnimation>
#include <QWidget>

// Sfondo slate con alone colorato che cambia con lo stato (transizione animata).
class Backdrop : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor accent READ accent WRITE setAccent)
public:
    explicit Backdrop(QWidget* parent = nullptr) : QWidget(parent), m_accent(100, 116, 139) {
        setObjectName(QStringLiteral("backdrop"));
    }

    QColor accent() const { return m_accent; }
    void setAccent(const QColor& c) { m_accent = c; update(); }

    void animateTo(const QColor& c) {
        auto* a = new QPropertyAnimation(this, "accent", this);
        a->setDuration(700);
        a->setEasingCurve(QEasingCurve::InOutCubic);
        a->setStartValue(m_accent);
        a->setEndValue(c);
        a->start(QAbstractAnimation::DeleteWhenStopped);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        QLinearGradient bg(0, 0, 0, height());
        bg.setColorAt(0.0, QColor(11, 16, 32));
        bg.setColorAt(1.0, QColor(15, 23, 42));
        p.fillRect(rect(), bg);

        QColor c = m_accent;
        QRadialGradient glow(width() / 2.0, 150, width() * 0.85);
        c.setAlpha(70);
        glow.setColorAt(0.0, c);
        c.setAlpha(0);
        glow.setColorAt(1.0, c);
        p.fillRect(rect(), glow);
    }

private:
    QColor m_accent;
};
