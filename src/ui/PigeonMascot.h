#pragma once
#include <QPixmap>
#include <QTimer>
#include <QWidget>
#include "core/VpnTypes.h"

// Piccione disegnato interamente con QPainter/QPainterPath (nessun font emoji, nessun PNG).
//  Disconnected: colori spenti, occhio chiuso (dorme), respiro lento
//  Connecting:   sbatte l'ala, dondola, guarda a destra e sinistra, punti arancioni in orbita
//  Connected:    postura calma, riflesso iridescente, anelli radar, scia di volo, badge-globo
//  Error:        tinta rossa, occhi a X, badge "!"
class PigeonMascot : public QWidget {
    Q_OBJECT
public:
    using State = VpnState;

    explicit PigeonMascot(QWidget* parent = nullptr);

    void setState(State s);
    State state() const { return m_state; }

    // Disegna il piccione scalato a `size` (coordinate logiche 120x120). Usato anche per l'icona del tray.
    static void paintPigeon(QPainter& p, State s, double phase, const QSizeF& size);
    static QPixmap renderPixmap(State s, int px, double phase = 0.8);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    State m_state = State::Disconnected;
    double m_phase = 0.0;
};
