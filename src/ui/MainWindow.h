#pragma once
#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include "core/VpnTypes.h"
#include "core/WireGuardConfig.h"

class Backdrop;
class IVpnBackend;
class PigeonMascot;
class Sparkline;
class QAction;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QSystemTrayIcon;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* e) override;

private:
    void buildUi();
    QWidget* buildTunnelPage();
    QWidget* buildSettingsPage();
    void buildTray();

    void createBackend(bool simulated);
    void loadSettings();
    void saveSettings() const;
    void loadNodeFields();
    TunnelConfig currentConfig() const;

    void toggleConnection();
    void requestQuit();
    void onState(VpnState s, const QString& detail);
    void onTotals(quint64 rx, quint64 tx);
    void appendLog(const QString& line);
    void updateKillSwitchLabel();
    void updateUptime();

    // UI
    Backdrop* m_backdrop = nullptr;
    PigeonMascot* m_mascot = nullptr;
    Sparkline* m_spark = nullptr;
    QLabel *m_status = nullptr, *m_detail = nullptr, *m_uptime = nullptr, *m_modeBadge = nullptr;
    QLabel *m_rxLabel = nullptr, *m_txLabel = nullptr, *m_totLabel = nullptr, *m_ksLabel = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QComboBox* m_nodeCombo = nullptr;
    QCheckBox *m_killSwitch = nullptr, *m_dryRun = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QLineEdit *m_privKey = nullptr, *m_psk = nullptr, *m_address = nullptr, *m_dns = nullptr;
    QLineEdit *m_endpoint = nullptr, *m_pubKey = nullptr;
    QSpinBox* m_mtu = nullptr;

    // Tray
    QSystemTrayIcon* m_tray = nullptr;
    QAction* m_trayToggle = nullptr;
    QTimer m_trayIconTimer;

    // Stato
    IVpnBackend* m_backend = nullptr;
    VpnState m_state = VpnState::Disconnected;
    QVector<ExitNode> m_nodes;
    bool m_loading = false;
    bool m_quitting = false;
    bool m_trayHintShown = false;

    QElapsedTimer m_uptimeClock, m_rateClock;
    QTimer m_uptimeTimer;
    bool m_haveLast = false;
    quint64 m_lastRx = 0, m_lastTx = 0;
};
