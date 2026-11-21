#include "ui/PigeonMascot.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {

constexpr double kTwoPi = 6.283185307179586;

QColor mixColor(const QColor& a, const QColor& b, double t) {
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t,
                            a.alphaF());
}

} // namespace

PigeonMascot::PigeonMascot(QWidget* parent) : QWidget(parent) {
    setFixedSize(120, 120);
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        m_phase += (m_state == State::Connecting) ? 0.18 : 0.06;
        if (m_phase > kTwoPi) m_phase -= kTwoPi;
        update();
    });
    timer->start(16);
}

void PigeonMascot::setState(State s) {
    m_state = s;
    update();
}

void PigeonMascot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    paintPigeon(p, m_state, m_phase, size());
}

QPixmap PigeonMascot::renderPixmap(State s, int px, double phase) {
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintPigeon(p, s, phase, QSizeF(px, px));
    return pm;
}

void PigeonMascot::paintPigeon(QPainter& p, State st, double phase, const QSizeF& size) {
    p.setRenderHint(QPainter::Antialiasing, true);
    p.save();
    p.scale(size.width() / 120.0, size.height() / 120.0);

    const bool off = st == State::Disconnected;
    const bool busy = st == State::Connecting;
    const bool on = st == State::Connected;
    const bool err = st == State::Error;

    // Adatta una tinta allo stato: spento = desaturato e piu' scuro, errore = virato al rosso.
    auto tone = [&](QColor c) {
        if (off) {
            QColor h = c.toHsl();
            h.setHsl(h.hue(), int(h.saturation() * 0.25), int(h.lightness() * 0.82), c.alpha());
            return h;
        }
        if (err) return mixColor(c, QColor(239, 68, 68, c.alpha()), 0.28);
        return c;
    };

    const double bob = busy ? qSin(phase * 2.0) * 5.0 : qSin(phase) * (off ? 1.2 : 2.0);

    p.setPen(Qt::NoPen);

    // Anelli radar (connesso), centrati sul piccione ma senza dondolio
    if (on) {
        p.save();
        p.translate(60.0, 64.0);
        for (int i = 0; i < 3; ++i) {
            const double t = std::fmod(phase / kTwoPi + i / 3.0, 1.0);
            QColor c(34, 211, 238, int((1.0 - t) * 95));
            p.setPen(QPen(c, 1.6));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(0, 0), 30.0 + t * 26.0, 30.0 + t * 26.0);
        }
        p.restore();
    }

    // Puntini in orbita (connessione in corso)
    if (busy) {
        p.save();
        p.translate(60.0, 64.0);
        p.setPen(Qt::NoPen);
        for (int i = 0; i < 3; ++i) {
            const double a = phase * 2.0 + i * kTwoPi / 3.0;
            p.setBrush(QColor(251, 191, 36, 210 - i * 50));
            p.drawEllipse(QPointF(qCos(a) * 50.0, qSin(a) * 50.0), 3.2 - i * 0.5, 3.2 - i * 0.5);
        }
        p.restore();
    }

    p.translate(60.0, 64.0 + bob);

    // Alone (connesso)
    if (on) {
        QRadialGradient glow(0, 0, 52);
        glow.setColorAt(0.0, QColor(34, 211, 238, 70));
        glow.setColorAt(1.0, QColor(34, 211, 238, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(QPointF(0, 0), 52, 52);

        // Scia di volo dietro la coda
        for (int i = 0; i < 3; ++i) {
            const double len = 6.0 + 4.0 * qSin(phase * 2.0 + i);
            p.setPen(QPen(QColor(34, 211, 238, 130 - i * 25), 2.0, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(-47 - len, -2 + i * 7), QPointF(-47, -2 + i * 7));
        }
    }

    // Coda
    QPainterPath tail;
    tail.moveTo(-22, 8);
    tail.lineTo(-42, 16);
    tail.lineTo(-36, 4);
    tail.lineTo(-44, -2);
    tail.lineTo(-22, 0);
    tail.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(tone(QColor(71, 85, 105)));
    p.drawPath(tail);

    // Corpo e testa
    QLinearGradient bodyGrad(-20, -32, 24, 28);
    bodyGrad.setColorAt(0.0, tone(QColor(148, 163, 184)));
    bodyGrad.setColorAt(0.55, tone(QColor(100, 116, 139)));
    bodyGrad.setColorAt(1.0, tone(QColor(71, 85, 105)));

    QPainterPath body;
    body.moveTo(-24, 6);
    body.cubicTo(-24, -14, -8, -18, 4, -18);
    body.cubicTo(6, -34, 26, -34, 26, -18);
    body.cubicTo(26, -8, 18, -4, 16, 2);
    body.cubicTo(22, 14, 10, 24, -6, 24);
    body.cubicTo(-18, 24, -24, 16, -24, 6);
    body.closeSubpath();

    p.setBrush(bodyGrad);
    p.setPen(QPen(tone(QColor(226, 232, 240, 140)), 1.5));
    p.drawPath(body);

    // Collo iridescente: la sfumatura "respira" con la fase
    const double shift = 0.5 + 0.5 * qSin(phase * 1.5);
    QLinearGradient neck(8, -22, 24, -6);
    neck.setColorAt(0.0, tone(QColor(16, 185, 129, 170)));
    neck.setColorAt(qBound(0.2, 0.3 + shift * 0.4, 0.8), tone(QColor(99, 102, 241, 170)));
    neck.setColorAt(1.0, tone(QColor(217, 70, 239, 140)));
    p.setPen(Qt::NoPen);
    p.setBrush(neck);
    p.drawEllipse(QRectF(10, -20, 14, 14));

    // Ala (sbatte quando si connette)
    const double wingAngle = busy ? qSin(phase * 3.0) * 18.0 - 8.0 : (off ? 2.0 : -4.0);
    p.save();
    p.translate(-4, 2);
    p.rotate(wingAngle);
    QPainterPath wing;
    wing.moveTo(0, -6);
    wing.quadTo(-18, -10, -22, 6);
    wing.quadTo(-8, 12, 4, 4);
    wing.closeSubpath();
    p.setBrush(tone(QColor(203, 213, 225)));
    p.drawPath(wing);
    p.restore();

    // Becco
    QPainterPath beak;
    beak.moveTo(25, -22);
    beak.lineTo(35, -18);
    beak.lineTo(25, -14);
    beak.closeSubpath();
    p.setBrush(tone(QColor(251, 191, 36)));
    p.drawPath(beak);

    // Occhio
    const QPointF eye(17, -22);
    if (off) {
        p.setPen(QPen(QColor(15, 23, 42), 2.0, Qt::SolidLine, Qt::RoundCap));
        QPainterPath lid;
        lid.moveTo(eye.x() - 4, eye.y());
        lid.quadTo(eye.x(), eye.y() + 3.5, eye.x() + 4, eye.y());
        p.setBrush(Qt::NoBrush);
        p.drawPath(lid);
    } else if (err) {
        p.setPen(QPen(QColor(15, 23, 42), 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(eye + QPointF(-3, -3), eye + QPointF(3, 3));
        p.drawLine(eye + QPointF(-3, 3), eye + QPointF(3, -3));
    } else {
        const double look = busy ? qSin(phase * 2.0) * 1.3 : 0.0;
        p.setBrush(QColor(15, 23, 42));
        p.drawEllipse(QPointF(eye.x() + look, eye.y()), 4.2, 4.2);
        p.setBrush(QColor(248, 250, 252));
        p.drawEllipse(QPointF(eye.x() + look + 1.2, eye.y() - 1.4), 1.4, 1.4);
    }

    // Zampe
    p.setPen(QPen(tone(QColor(249, 115, 22)), 2.5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-4, 24), QPointF(-4, 32));
    p.drawLine(QPointF(-4, 32), QPointF(2, 34));
    p.drawLine(QPointF(6, 24), QPointF(6, 32));
    p.drawLine(QPointF(6, 32), QPointF(12, 34));

    // Badge: globo (connesso) oppure "!" (errore)
    if (on || err) {
        const QPointF c(-31, -30);
        p.setPen(QPen(QColor(15, 23, 42), 1.5));
        p.setBrush(err ? QColor(239, 68, 68) : QColor(34, 211, 238));
        p.drawEllipse(c, 10, 10);
        p.setPen(QPen(QColor(248, 250, 252), 1.4, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        if (on) {
            p.drawEllipse(c, 5.5, 5.5);
            p.drawEllipse(c, 2.4, 5.5);
            p.drawLine(c + QPointF(-5.5, 0), c + QPointF(5.5, 0));
        } else {
            p.setPen(QPen(QColor(248, 250, 252), 2.2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(c + QPointF(0, -5), c + QPointF(0, 1.5));
            p.drawPoint(c + QPointF(0, 5));
        }
    }

    p.restore();
}
