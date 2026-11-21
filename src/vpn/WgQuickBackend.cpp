#include "vpn/WgQuickBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

namespace {

QString findTool(const QString& name, const QStringList& extraDirs) {
    const QString p = QStandardPaths::findExecutable(name);
    if (!p.isEmpty()) return p;
    return QStandardPaths::findExecutable(name, extraDirs);
}

#if defined(Q_OS_MACOS)
bool shellSafe(const QString& s) {
    static const QRegularExpression ok(QStringLiteral(R"(^[A-Za-z0-9_./\-]+$)"));
    return ok.match(s).hasMatch();
}
#endif

} // namespace

WgQuickBackend::WgQuickBackend(QObject* parent) : IVpnBackend(parent) {
    m_poll.setInterval(1000);
    connect(&m_poll, &QTimer::timeout, this, &WgQuickBackend::pollStats);
}

WgQuickBackend::~WgQuickBackend() { wipeConfig(); }

void WgQuickBackend::setState(VpnState s, const QString& detail) {
    m_state = s;
    emit stateChanged(s, detail);
}

WgQuickBackend::Command WgQuickBackend::buildCommand(bool up) const {
    Command c;
#if defined(Q_OS_WIN)
    const QString exe = findTool(QStringLiteral("wireguard"),
                                 {QStringLiteral("C:/Program Files/WireGuard")});
    if (exe.isEmpty()) { c.error = QStringLiteral("wireguard.exe non trovato (installa WireGuard per Windows)."); return c; }
    c.program = exe;
    c.args = up ? QStringList{QStringLiteral("/installtunnelservice"), QDir::toNativeSeparators(m_confPath)}
                : QStringList{QStringLiteral("/uninstalltunnelservice"), m_iface};
#elif defined(Q_OS_MACOS)
    const QString tool = findTool(QStringLiteral("wg-quick"),
                                  {QStringLiteral("/opt/homebrew/bin"), QStringLiteral("/usr/local/bin")});
    if (tool.isEmpty()) { c.error = QStringLiteral("wg-quick non trovato (brew install wireguard-tools)."); return c; }
    if (!shellSafe(tool) || !shellSafe(m_confPath)) { c.error = QStringLiteral("Percorso con caratteri non sicuri."); return c; }
    const QString script = QStringLiteral(
        "export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH; %1 %2 %3").arg(tool, up ? "up" : "down", m_confPath);
    c.program = QStringLiteral("osascript");
    c.args = {QStringLiteral("-e"),
              QStringLiteral("do shell script \"%1\" with administrator privileges").arg(script)};
#else
    const QStringList dirs{QStringLiteral("/usr/bin"), QStringLiteral("/usr/sbin"),
                           QStringLiteral("/usr/local/bin"), QStringLiteral("/usr/local/sbin")};
    const QString tool = findTool(QStringLiteral("wg-quick"), dirs);
    const QString pk = findTool(QStringLiteral("pkexec"), dirs);
    if (tool.isEmpty()) { c.error = QStringLiteral("wg-quick non trovato (installa wireguard-tools)."); return c; }
    if (pk.isEmpty()) { c.error = QStringLiteral("pkexec non trovato (installa polkit)."); return c; }
    c.program = pk;
    c.args = {tool, up ? QStringLiteral("up") : QStringLiteral("down"), m_confPath};
#endif
    return c;
}

bool WgQuickBackend::writeConfig(const TunnelConfig& cfg, QString* err) {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_dir = base + QStringLiteral("/pigeonvpn-") + QUuid::createUuid().toString(QUuid::Id128).left(12);
    if (!QDir().mkpath(m_dir)) { *err = QStringLiteral("Impossibile creare la directory temporanea."); return false; }
    QFile::setPermissions(m_dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    m_confPath = m_dir + QLatin1Char('/') + m_iface + QStringLiteral(".conf");
    QFile f(m_confPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { *err = QStringLiteral("Impossibile scrivere la configurazione."); return false; }
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#if defined(Q_OS_LINUX)
    const bool linuxKs = true;
#else
    const bool linuxKs = false;
#endif
    f.write(cfg.toWgQuick(linuxKs).toUtf8());
    f.close();
    return true;
}

void WgQuickBackend::wipeConfig() {
    if (!m_confPath.isEmpty()) {
        QFile f(m_confPath);
        if (f.exists() && f.open(QIODevice::ReadWrite)) {
            const qint64 n = f.size();
            f.seek(0);
            f.write(QByteArray(static_cast<int>(n), '\0'));
            f.flush();
            f.close();
        }
        QFile::remove(m_confPath);
    }
    if (!m_dir.isEmpty()) QDir().rmdir(m_dir);
    m_confPath.clear();
    m_dir.clear();
}

void WgQuickBackend::connectTunnel(const TunnelConfig& cfg) {
    if (m_state == VpnState::Connecting || m_state == VpnState::Connected) return;
    m_pendingDisconnect = false;
    m_statsWarned = false;

    const QString verr = cfg.validate();
    if (!verr.isEmpty()) { setState(VpnState::Error, verr); return; }

#if !defined(Q_OS_LINUX)
    if (cfg.killSwitch) log(QStringLiteral("Kill-switch automatico supportato solo su Linux: ignorato su questa piattaforma."));
#endif
    setState(VpnState::Connecting, cfg.node.name);
    log(cfg.redactedSummary());

    // Copia SENZA chiavi, usata per "down": wg-quick down ha bisogno del file (hook, DNS, kill-switch)
    // ma non della chiave privata, che cosi' non viene mai riscritta su disco.
    m_downCfg = cfg;
    m_downCfg.privateKey.clear();
    m_downCfg.presharedKey.clear();

    QString err;
    if (!writeConfig(cfg, &err)) { wipeConfig(); setState(VpnState::Error, err); return; }
    runElevated(true);
}

void WgQuickBackend::disconnectTunnel() {
    if (m_state == VpnState::Disconnected) return;
    if (m_proc) { m_pendingDisconnect = true; return; } // un comando e' in corso: lo si accoda
    if (m_state == VpnState::Error) { setState(VpnState::Disconnected); return; }
    m_poll.stop();
    QString err;
    if (!writeConfig(m_downCfg, &err)) { wipeConfig(); setState(VpnState::Error, err); return; }
    runElevated(false);
}

void WgQuickBackend::runElevated(bool up) {
    const Command cmd = buildCommand(up);
    if (!cmd.error.isEmpty()) {
        log(cmd.error);
        m_poll.stop();
        wipeConfig();
        setState(up ? VpnState::Error : VpnState::Disconnected, cmd.error);
        return;
    }
    log(QStringLiteral("Esecuzione: %1 %2").arg(QFileInfo(cmd.program).fileName(), up ? "up" : "down"));

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::finished, this, [this, up](int code, QProcess::ExitStatus st) {
        const QString out = QString::fromLocal8Bit(m_proc->readAll()).trimmed();
        m_proc->deleteLater();
        m_proc = nullptr;
        for (const QString& line : out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) log(line.trimmed());

        const bool ok = (st == QProcess::NormalExit && code == 0);
        if (up) {
            wipeConfig(); // la chiave privata non deve restare su disco
            if (!ok) { setState(VpnState::Error, QStringLiteral("Avvio tunnel fallito (codice %1).").arg(code)); return; }
            setState(VpnState::Connected);
            m_poll.start();
            if (m_pendingDisconnect) { m_pendingDisconnect = false; disconnectTunnel(); }
        } else {
            wipeConfig();
            setState(ok ? VpnState::Disconnected : VpnState::Error,
                     ok ? QString() : QStringLiteral("Chiusura tunnel fallita (codice %1).").arg(code));
        }
    });
    connect(m_proc, &QProcess::errorOccurred, this, [this, up](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart || !m_proc) return;
        m_proc->deleteLater();
        m_proc = nullptr;
        wipeConfig();
        setState(VpnState::Error, QStringLiteral("Impossibile avviare l'helper privilegiato."));
        Q_UNUSED(up)
    });
    m_proc->start(cmd.program, cmd.args);
}

void WgQuickBackend::pollStats() {
    if (m_statProc) return;
#if defined(Q_OS_LINUX)
    auto readCounter = [this](const char* name) -> quint64 {
        QFile f(QStringLiteral("/sys/class/net/%1/statistics/%2").arg(m_iface, QLatin1String(name)));
        if (!f.open(QIODevice::ReadOnly)) return 0;
        return f.readAll().trimmed().toULongLong();
    };
    emit trafficTotals(readCounter("rx_bytes"), readCounter("tx_bytes"));
#else
    const QString wg = findTool(QStringLiteral("wg"), {QStringLiteral("C:/Program Files/WireGuard"),
                                                       QStringLiteral("/opt/homebrew/bin"),
                                                       QStringLiteral("/usr/local/bin")});
    if (wg.isEmpty()) return;
    m_statProc = new QProcess(this);
    connect(m_statProc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        const QString out = QString::fromLocal8Bit(m_statProc->readAllStandardOutput());
        m_statProc->deleteLater();
        m_statProc = nullptr;
        if (code != 0) {
            if (!m_statsWarned) { m_statsWarned = true; log(QStringLiteral("Statistiche non disponibili senza privilegi (wg show).")); }
            return;
        }
        quint64 rx = 0, tx = 0; // output: <pubkey>\t<rx>\t<tx> per peer
        for (const QString& line : out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            const QStringList p = line.split(QLatin1Char('\t'));
            if (p.size() >= 3) { rx += p[1].toULongLong(); tx += p[2].toULongLong(); }
        }
        emit trafficTotals(rx, tx);
    });
    m_statProc->start(wg, {QStringLiteral("show"), m_iface, QStringLiteral("transfer")});
#endif
}
