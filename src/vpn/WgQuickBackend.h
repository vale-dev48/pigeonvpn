#pragma once
#include <QProcess>
#include <QTimer>
#include "vpn/IVpnBackend.h"

// Backend reale: delega a wg-quick (Linux/macOS) o wireguard.exe (Windows).
//
// Confine dei privilegi: la GUI NON gira da root. Solo il singolo comando
// "wg-quick up/down <file>" viene elevato (pkexec su Linux, osascript su macOS;
// su Windows l'app va avviata come amministratore perche' installa un servizio).
// La chiave privata esiste su disco solo nel file temporaneo 0600 in una directory 0700,
// sovrascritto con zeri e cancellato appena il tunnel e' su o fallisce.
class WgQuickBackend : public IVpnBackend {
    Q_OBJECT
public:
    explicit WgQuickBackend(QObject* parent = nullptr);
    ~WgQuickBackend() override;

    QString displayName() const override { return QStringLiteral("WireGuard reale (wg-quick)"); }
    bool isSimulated() const override { return false; }
    VpnState state() const override { return m_state; }

public slots:
    void connectTunnel(const TunnelConfig& cfg) override;
    void disconnectTunnel() override;

private:
    struct Command { QString program; QStringList args; QString error; };

    Command buildCommand(bool up) const;
    bool writeConfig(const TunnelConfig& cfg, QString* err);
    void wipeConfig();
    void runElevated(bool up);
    void pollStats();
    void setState(VpnState s, const QString& detail = {});
    void log(const QString& line) { emit logMessage(redactSecrets(line)); }

    VpnState m_state = VpnState::Disconnected;
    QString m_iface = QStringLiteral("pigeon0");
    TunnelConfig m_downCfg;  // senza chiavi
    QString m_dir;
    QString m_confPath;
    QProcess* m_proc = nullptr;
    QProcess* m_statProc = nullptr;
    QTimer m_poll;
    bool m_pendingDisconnect = false;
    bool m_statsWarned = false;
};
