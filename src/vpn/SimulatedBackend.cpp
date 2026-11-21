#include "vpn/SimulatedBackend.h"

#include <QRandomGenerator>

SimulatedBackend::SimulatedBackend(QObject* parent) : IVpnBackend(parent) {
    m_traffic.setInterval(1000);
    connect(&m_traffic, &QTimer::timeout, this, &SimulatedBackend::tick);
}

void SimulatedBackend::setState(VpnState s, const QString& detail) {
    m_state = s;
    emit stateChanged(s, detail);
}

void SimulatedBackend::connectTunnel(const TunnelConfig& cfg) {
    if (m_state == VpnState::Connecting || m_state == VpnState::Connected) return;
    const int gen = ++m_generation;
    setState(VpnState::Connecting, cfg.node.name);
    emit logMessage(QStringLiteral("[sim] %1").arg(cfg.redactedSummary()));
    emit logMessage(QStringLiteral("[sim] creazione interfaccia pigeon0 (finta)"));

    QTimer::singleShot(600, this, [this, gen, cfg] {
        if (gen != m_generation) return;
        emit logMessage(QStringLiteral("[sim] handshake initiation -> %1").arg(cfg.node.endpoint));
    });
    QTimer::singleShot(1500, this, [this, gen, cfg] {
        if (gen != m_generation) return;
        if (cfg.node.endpoint.startsWith(QStringLiteral("error.test"))) {
            emit logMessage(QStringLiteral("[sim] handshake fallito (timeout)"));
            setState(VpnState::Error, QStringLiteral("Handshake fallito: nessuna risposta dal peer"));
            return;
        }
        m_rxBps = m_txBps = 0;
        m_rxTotal = m_txTotal = 0;
        emit logMessage(QStringLiteral("[sim] handshake completato, tunnel attivo (AllowedIPs %1)")
                            .arg(TunnelConfig::allowedIps()));
        if (cfg.killSwitch) emit logMessage(QStringLiteral("[sim] kill-switch armato (simulato)"));
        m_traffic.start();
        setState(VpnState::Connected, cfg.node.name);
    });
}

void SimulatedBackend::disconnectTunnel() {
    if (m_state == VpnState::Disconnected) return;
    const int gen = ++m_generation;
    m_traffic.stop();
    emit logMessage(QStringLiteral("[sim] chiusura tunnel"));
    QTimer::singleShot(350, this, [this, gen] {
        if (gen != m_generation) return;
        setState(VpnState::Disconnected);
    });
}

void SimulatedBackend::tick() {
    auto* rng = QRandomGenerator::global();
    double target = 6e6 + rng->generateDouble() * 70e6;           // 6..76 Mbit/s
    if (rng->bounded(8) == 0) target += 120e6 * rng->generateDouble(); // burst occasionale
    m_rxBps = m_rxBps * 0.6 + target * 0.4;
    m_txBps = m_txBps * 0.6 + target * (0.08 + 0.12 * rng->generateDouble()) * 0.4;
    m_rxTotal += static_cast<quint64>(m_rxBps / 8.0);
    m_txTotal += static_cast<quint64>(m_txBps / 8.0);
    emit trafficTotals(m_rxTotal, m_txTotal);
}
