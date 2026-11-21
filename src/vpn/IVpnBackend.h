#pragma once
#include <QObject>
#include "core/VpnTypes.h"
#include "core/WireGuardConfig.h"

// Confine tra UI (non privilegiata) e operazioni di sistema (privilegiate).
// La UI non sa nulla di wg, pkexec, servizi Windows: parla solo con questa interfaccia.
class IVpnBackend : public QObject {
    Q_OBJECT
public:
    explicit IVpnBackend(QObject* parent = nullptr) : QObject(parent) {}
    ~IVpnBackend() override = default;

    virtual QString displayName() const = 0;
    virtual bool isSimulated() const = 0;
    virtual VpnState state() const = 0;

public slots:
    virtual void connectTunnel(const TunnelConfig& cfg) = 0;
    virtual void disconnectTunnel() = 0;

signals:
    void stateChanged(VpnState state, const QString& detail);
    // Contatori cumulativi dall'avvio del tunnel; la UI ne ricava i bitrate.
    void trafficTotals(quint64 rxBytes, quint64 txBytes);
    // Messaggi gia' ripuliti da segreti dal backend.
    void logMessage(const QString& line);
};
