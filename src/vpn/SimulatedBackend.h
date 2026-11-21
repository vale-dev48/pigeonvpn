#pragma once
#include <QTimer>
#include "vpn/IVpnBackend.h"

// Dry-run: nessun processo, nessun privilegio, nessuna rete. Simula handshake e traffico.
// Per provare lo stato Error imposta l'endpoint di un nodo su "error.test:51820".
class SimulatedBackend : public IVpnBackend {
    Q_OBJECT
public:
    explicit SimulatedBackend(QObject* parent = nullptr);

    QString displayName() const override { return QStringLiteral("Simulazione (dry-run)"); }
    bool isSimulated() const override { return true; }
    VpnState state() const override { return m_state; }

public slots:
    void connectTunnel(const TunnelConfig& cfg) override;
    void disconnectTunnel() override;

private:
    void setState(VpnState s, const QString& detail = {});
    void tick();

    VpnState m_state = VpnState::Disconnected;
    int m_generation = 0;  // invalida i singleShot pendenti dopo un disconnect
    QTimer m_traffic;
    double m_rxBps = 0, m_txBps = 0;
    quint64 m_rxTotal = 0, m_txTotal = 0;
};
