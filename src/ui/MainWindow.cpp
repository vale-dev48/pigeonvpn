#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QVBoxLayout>

#include "core/Format.h"
#include "ui/Backdrop.h"
#include "ui/PigeonMascot.h"
#include "ui/Sparkline.h"
#include "vpn/IVpnBackend.h"
#include "vpn/SimulatedBackend.h"
#include "vpn/WgQuickBackend.h"

namespace {

QFrame* makeCard() {
    auto* f = new QFrame;
    f->setObjectName(QStringLiteral("card"));
    return f;
}

void setProp(QWidget* w, const char* name, const QString& value) {
    w->setProperty(name, value);
    w->style()->unpolish(w);
    w->style()->polish(w);
}

QVector<ExitNode> defaultNodes() {
    // Endpoint nel range documentazione 203.0.113.0/24 (RFC 5737): sostituisci con i tuoi server reali.
    return {
        {QStringLiteral("Frankfurt"), QStringLiteral("DE"), QStringLiteral("203.0.113.10:51820"), {}},
        {QStringLiteral("Tokyo"), QStringLiteral("JP"), QStringLiteral("203.0.113.20:51820"), {}},
        {QStringLiteral("New York"), QStringLiteral("US"), QStringLiteral("203.0.113.30:51820"), {}},
        {QStringLiteral("São Paulo"), QStringLiteral("BR"), QStringLiteral("203.0.113.40:51820"), {}},
        {QStringLiteral("Milan"), QStringLiteral("IT"), QStringLiteral("203.0.113.50:51820"), {}},
    };
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("PigeonVPN"));
    resize(540, 900);
    setMinimumWidth(480);

    buildUi();
    buildTray();
    loadSettings();
    createBackend(m_dryRun->isChecked());
    onState(VpnState::Disconnected, {});

    m_uptimeTimer.setInterval(1000);
    connect(&m_uptimeTimer, &QTimer::timeout, this, &MainWindow::updateUptime);

    appendLog(QStringLiteral("PigeonVPN pronto. Le chiavi private non vengono mai scritte nei log né salvate nelle impostazioni."));
}

MainWindow::~MainWindow() {
    saveSettings();
    delete m_backend;
    m_backend = nullptr;
}

// ---------------------------------------------------------------- UI

void MainWindow::buildUi() {
    m_backdrop = new Backdrop;
    setCentralWidget(m_backdrop);

    auto* root = new QVBoxLayout(m_backdrop);
    root->setContentsMargins(22, 18, 22, 18);
    root->setSpacing(10);

    auto* header = new QHBoxLayout;
    auto* titles = new QVBoxLayout;
    titles->setSpacing(0);
    auto* title = new QLabel(QStringLiteral("PigeonVPN"));
    title->setObjectName(QStringLiteral("title"));
    auto* sub = new QLabel(QStringLiteral("Messaggero globale · WireGuard"));
    sub->setObjectName(QStringLiteral("subtitle"));
    titles->addWidget(title);
    titles->addWidget(sub);
    m_modeBadge = new QLabel;
    m_modeBadge->setObjectName(QStringLiteral("badge"));
    header->addLayout(titles);
    header->addStretch();
    header->addWidget(m_modeBadge, 0, Qt::AlignTop);
    root->addLayout(header);

    auto* tabs = new QTabWidget;
    tabs->addTab(buildTunnelPage(), QStringLiteral("Tunnel"));
    tabs->addTab(buildSettingsPage(), QStringLiteral("Configurazione"));
    root->addWidget(tabs, 1);
}

QWidget* MainWindow::buildTunnelPage() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(0, 10, 0, 0);
    v->setSpacing(12);

    // --- Hero
    auto* hero = makeCard();
    auto* hv = new QVBoxLayout(hero);
    hv->setContentsMargins(20, 16, 20, 20);
    hv->setSpacing(4);

    m_mascot = new PigeonMascot;
    m_mascot->setFixedSize(170, 170);
    hv->addWidget(m_mascot, 0, Qt::AlignHCenter);

    m_status = new QLabel;
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setAlignment(Qt::AlignCenter);
    m_detail = new QLabel;
    m_detail->setObjectName(QStringLiteral("detail"));
    m_detail->setAlignment(Qt::AlignCenter);
    m_detail->setWordWrap(true);
    m_uptime = new QLabel(QStringLiteral("00:00:00"));
    m_uptime->setObjectName(QStringLiteral("uptime"));
    m_uptime->setAlignment(Qt::AlignCenter);
    hv->addWidget(m_status);
    hv->addWidget(m_detail);
    hv->addSpacing(2);
    hv->addWidget(m_uptime);
    hv->addSpacing(8);

    m_connectBtn = new QPushButton;
    m_connectBtn->setObjectName(QStringLiteral("connectBtn"));
    m_connectBtn->setCursor(Qt::PointingHandCursor);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    hv->addWidget(m_connectBtn);
    v->addWidget(hero);

    // --- Nodo + kill-switch
    auto* nodeCard = makeCard();
    auto* nv = new QVBoxLayout(nodeCard);
    nv->setContentsMargins(18, 14, 18, 14);
    nv->setSpacing(8);
    auto* nt = new QLabel(QStringLiteral("NODO DI USCITA"));
    nt->setObjectName(QStringLiteral("cardTitle"));
    m_nodeCombo = new QComboBox;
    connect(m_nodeCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        if (!m_loading) loadNodeFields();
    });
    auto* ksRow = new QHBoxLayout;
    m_killSwitch = new QCheckBox(QStringLiteral("Kill-switch"));
    m_killSwitch->setToolTip(QStringLiteral("Blocca il traffico fuori dal tunnel (regole firewall automatiche solo su Linux)."));
    connect(m_killSwitch, &QCheckBox::toggled, this, [this] { updateKillSwitchLabel(); });
    m_ksLabel = new QLabel;
    m_ksLabel->setObjectName(QStringLiteral("ks"));
    ksRow->addWidget(m_killSwitch);
    ksRow->addStretch();
    ksRow->addWidget(m_ksLabel);
    nv->addWidget(nt);
    nv->addWidget(m_nodeCombo);
    nv->addLayout(ksRow);
    v->addWidget(nodeCard);

    // --- Telemetria
    auto* tele = makeCard();
    auto* tv = new QVBoxLayout(tele);
    tv->setContentsMargins(18, 14, 18, 12);
    tv->setSpacing(6);
    auto* tt = new QLabel(QStringLiteral("TRAFFICO IN TEMPO REALE"));
    tt->setObjectName(QStringLiteral("cardTitle"));
    tv->addWidget(tt);

    auto* rates = new QHBoxLayout;
    m_rxLabel = new QLabel;
    m_rxLabel->setObjectName(QStringLiteral("rxValue"));
    m_txLabel = new QLabel;
    m_txLabel->setObjectName(QStringLiteral("txValue"));
    rates->addWidget(m_rxLabel);
    rates->addStretch();
    rates->addWidget(m_txLabel);
    tv->addLayout(rates);

    m_spark = new Sparkline;
    tv->addWidget(m_spark);
    m_totLabel = new QLabel;
    m_totLabel->setObjectName(QStringLiteral("totValue"));
    tv->addWidget(m_totLabel);
    v->addWidget(tele);

    // --- Log
    m_log = new QPlainTextEdit;
    m_log->setObjectName(QStringLiteral("log"));
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);
    m_log->setMinimumHeight(90);
    v->addWidget(m_log, 1);

    return page;
}

QWidget* MainWindow::buildSettingsPage() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(0, 10, 0, 0);

    auto* card = makeCard();
    auto* form = new QFormLayout(card);
    form->setContentsMargins(18, 16, 18, 16);
    form->setVerticalSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft);

    m_dryRun = new QCheckBox(QStringLiteral("Modalità simulazione (nessun privilegio, nessuna rete)"));
    m_dryRun->setChecked(true);
    connect(m_dryRun, &QCheckBox::toggled, this, [this](bool sim) {
        if (m_loading) return;
        createBackend(sim);
        onState(VpnState::Disconnected, {});
        appendLog(sim ? QStringLiteral("Passato alla modalità simulazione.")
                      : QStringLiteral("Modalità REALE: richiede wg-quick + privilegi e chiavi valide."));
    });

    m_privKey = new QLineEdit;
    m_privKey->setEchoMode(QLineEdit::Password);
    m_privKey->setPlaceholderText(QStringLiteral("solo in memoria, non viene salvata"));
    m_psk = new QLineEdit;
    m_psk->setEchoMode(QLineEdit::Password);
    m_psk->setPlaceholderText(QStringLiteral("opzionale, solo in memoria"));
    m_address = new QLineEdit;
    m_dns = new QLineEdit;
    m_mtu = new QSpinBox;
    m_mtu->setRange(1280, 1500);
    m_mtu->setValue(1420);
    m_endpoint = new QLineEdit;
    m_endpoint->setPlaceholderText(QStringLiteral("host:porta"));
    m_pubKey = new QLineEdit;
    m_pubKey->setPlaceholderText(QStringLiteral("chiave pubblica del peer (base64)"));
    auto* allowed = new QLabel(TunnelConfig::allowedIps() + QStringLiteral("  (full tunnel)"));
    allowed->setObjectName(QStringLiteral("detail"));

    connect(m_endpoint, &QLineEdit::textEdited, this, [this](const QString& t) {
        const int i = m_nodeCombo->currentIndex();
        if (i >= 0) m_nodes[i].endpoint = t.trimmed();
    });
    connect(m_pubKey, &QLineEdit::textEdited, this, [this](const QString& t) {
        const int i = m_nodeCombo->currentIndex();
        if (i >= 0) m_nodes[i].publicKey = t.trimmed();
    });

    form->addRow(m_dryRun);
    form->addRow(QStringLiteral("PrivateKey"), m_privKey);
    form->addRow(QStringLiteral("PresharedKey"), m_psk);
    form->addRow(QStringLiteral("Address"), m_address);
    form->addRow(QStringLiteral("DNS"), m_dns);
    form->addRow(QStringLiteral("MTU"), m_mtu);
    form->addRow(QStringLiteral("AllowedIPs"), allowed);
    auto* sep = new QLabel(QStringLiteral("NODO SELEZIONATO (peer)"));
    sep->setObjectName(QStringLiteral("cardTitle"));
    form->addRow(sep);
    form->addRow(QStringLiteral("Endpoint"), m_endpoint);
    form->addRow(QStringLiteral("PublicKey"), m_pubKey);

    v->addWidget(card);
    v->addStretch();
    return page;
}

void MainWindow::buildTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon(PigeonMascot::renderPixmap(VpnState::Disconnected, 64)));
    m_tray->setToolTip(QStringLiteral("PigeonVPN"));

    auto* menu = new QMenu(this);
    auto* showAct = menu->addAction(QStringLiteral("Mostra / nascondi"));
    connect(showAct, &QAction::triggered, this, [this] {
        if (isVisible()) hide(); else { show(); raise(); activateWindow(); }
    });
    m_trayToggle = menu->addAction(QStringLiteral("Connetti"));
    connect(m_trayToggle, &QAction::triggered, this, &MainWindow::toggleConnection);
    menu->addSeparator();
    auto* quit = menu->addAction(QStringLiteral("Esci"));
    connect(quit, &QAction::triggered, this, &MainWindow::requestQuit);
    m_tray->setContextMenu(menu);

    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r != QSystemTrayIcon::Trigger) return;
        if (isVisible()) hide(); else { show(); raise(); activateWindow(); }
    });
    m_tray->show();
}

// ---------------------------------------------------------------- Backend

void MainWindow::createBackend(bool simulated) {
    if (m_backend) { delete m_backend; m_backend = nullptr; }
    if (simulated) m_backend = new SimulatedBackend;
    else m_backend = new WgQuickBackend;

    connect(m_backend, &IVpnBackend::stateChanged, this, &MainWindow::onState);
    connect(m_backend, &IVpnBackend::trafficTotals, this, &MainWindow::onTotals);
    connect(m_backend, &IVpnBackend::logMessage, this, &MainWindow::appendLog);

    m_modeBadge->setText(simulated ? QStringLiteral("SIMULAZIONE") : QStringLiteral("REALE"));
    setProp(m_modeBadge, "sim", simulated ? QStringLiteral("true") : QStringLiteral("false"));
}

TunnelConfig MainWindow::currentConfig() const {
    TunnelConfig c;
    c.privateKey = m_privKey->text().trimmed();
    c.presharedKey = m_psk->text().trimmed();
    c.address = m_address->text().trimmed();
    c.dns = m_dns->text().trimmed();
    c.mtu = m_mtu->value();
    c.killSwitch = m_killSwitch->isChecked();
    const int i = m_nodeCombo->currentIndex();
    if (i >= 0 && i < m_nodes.size()) c.node = m_nodes[i];
    return c;
}

void MainWindow::toggleConnection() {
    if (!m_backend) return;
    if (m_state == VpnState::Disconnected || m_state == VpnState::Error)
        m_backend->connectTunnel(currentConfig());
    else
        m_backend->disconnectTunnel();
}

void MainWindow::requestQuit() {
    m_quitting = true;
    saveSettings();
    if (m_backend && (m_state == VpnState::Connected || m_state == VpnState::Connecting)) {
        appendLog(QStringLiteral("Chiusura: disconnessione del tunnel..."));
        m_backend->disconnectTunnel();
        QTimer::singleShot(10000, qApp, &QCoreApplication::quit); // rete di sicurezza
    } else {
        qApp->quit();
    }
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (m_tray && m_tray->isVisible() && !m_quitting) {
        hide();
        if (!m_trayHintShown) {
            m_trayHintShown = true;
            m_tray->showMessage(QStringLiteral("PigeonVPN"),
                                QStringLiteral("Resto attivo nell'area di notifica. Usa 'Esci' dal menu per chiudere."),
                                QSystemTrayIcon::Information, 3000);
        }
        e->ignore();
        return;
    }
    e->accept();
    if (!m_quitting) requestQuit();
}

// ---------------------------------------------------------------- Stato

void MainWindow::onState(VpnState s, const QString& detail) {
    m_state = s;
    m_mascot->setState(s);

    QString key, status, btn;
    QColor accent;
    switch (s) {
    case VpnState::Disconnected:
        key = QStringLiteral("disconnected"); status = QStringLiteral("Non protetto");
        btn = QStringLiteral("Connetti"); accent = QColor(100, 116, 139); break;
    case VpnState::Connecting:
        key = QStringLiteral("connecting"); status = QStringLiteral("Connessione in corso…");
        btn = QStringLiteral("Annulla"); accent = QColor(251, 191, 36); break;
    case VpnState::Connected:
        key = QStringLiteral("connected"); status = QStringLiteral("Protetto");
        btn = QStringLiteral("Disconnetti"); accent = QColor(34, 211, 238); break;
    case VpnState::Error:
        key = QStringLiteral("error"); status = QStringLiteral("Errore");
        btn = QStringLiteral("Riprova"); accent = QColor(248, 113, 113); break;
    }

    m_status->setText(status);
    m_connectBtn->setText(btn);
    setProp(m_status, "vpn", key);
    setProp(m_connectBtn, "vpn", key);
    m_backdrop->animateTo(accent);

    const int ni = m_nodeCombo->currentIndex();
    const QString nodeName = (ni >= 0 && ni < m_nodes.size()) ? m_nodes[ni].label() : QString();
    if (s == VpnState::Connected) m_detail->setText(QStringLiteral("Uscita globale via %1").arg(nodeName));
    else if (s == VpnState::Connecting) m_detail->setText(QStringLiteral("Handshake con %1").arg(nodeName));
    else if (s == VpnState::Error) m_detail->setText(detail.isEmpty() ? QStringLiteral("Errore sconosciuto") : detail);
    else m_detail->setText(QStringLiteral("Il tuo traffico esce dalla rete locale"));

    const bool idle = (s == VpnState::Disconnected || s == VpnState::Error);
    m_nodeCombo->setEnabled(idle);
    m_dryRun->setEnabled(idle);
    m_privKey->setEnabled(idle);
    m_psk->setEnabled(idle);
    m_address->setEnabled(idle);
    m_dns->setEnabled(idle);
    m_mtu->setEnabled(idle);
    m_endpoint->setEnabled(idle);
    m_pubKey->setEnabled(idle);
    m_killSwitch->setEnabled(idle);

    if (s == VpnState::Connected) {
        m_uptimeClock.start();
        m_uptimeTimer.start();
        m_haveLast = false;
        updateUptime();
    } else {
        m_uptimeTimer.stop();
        if (idle) {
            m_uptime->setText(QStringLiteral("00:00:00"));
            m_spark->clear();
            m_rxLabel->setText(QStringLiteral("↓ 0 bit/s"));
            m_txLabel->setText(QStringLiteral("↑ 0 bit/s"));
            m_totLabel->setText(QStringLiteral("Totale  ↓ 0 B   ↑ 0 B"));
            m_haveLast = false;
        }
    }
    updateKillSwitchLabel();

    if (m_tray) {
        m_tray->setIcon(QIcon(PigeonMascot::renderPixmap(s, 64)));
        m_tray->setToolTip(QStringLiteral("PigeonVPN — %1").arg(status));
        m_trayToggle->setText(idle ? QStringLiteral("Connetti") : QStringLiteral("Disconnetti"));
    }

    appendLog(QStringLiteral("Stato: %1%2").arg(toString(s), detail.isEmpty() || s == VpnState::Connected
                                                                ? QString() : QStringLiteral(" — ") + detail));
    if (m_quitting && idle) qApp->quit();
}

void MainWindow::updateKillSwitchLabel() {
    const bool want = m_killSwitch->isChecked();
    const bool sim = m_backend && m_backend->isSimulated();
    QString text;
    bool on = false;
    if (!want) text = QStringLiteral("○ Kill-switch spento");
    else if (m_state == VpnState::Connected) { text = sim ? QStringLiteral("● Kill-switch attivo (simulato)") : QStringLiteral("● Kill-switch attivo"); on = true; }
    else text = QStringLiteral("◐ Kill-switch: si arma alla connessione");
    m_ksLabel->setText(text);
    setProp(m_ksLabel, "on", on ? QStringLiteral("true") : QStringLiteral("false"));
}

void MainWindow::updateUptime() {
    m_uptime->setText(formatUptime(m_uptimeClock.elapsed() / 1000));
}

void MainWindow::onTotals(quint64 rx, quint64 tx) {
    if (m_state != VpnState::Connected) return;
    if (m_haveLast && rx >= m_lastRx && tx >= m_lastTx) {
        const double dt = m_rateClock.restart() / 1000.0;
        if (dt > 0.05) {
            const double rxBps = (rx - m_lastRx) * 8.0 / dt;
            const double txBps = (tx - m_lastTx) * 8.0 / dt;
            m_spark->push(rxBps, txBps);
            m_rxLabel->setText(QStringLiteral("↓ %1").arg(formatBitrate(rxBps)));
            m_txLabel->setText(QStringLiteral("↑ %1").arg(formatBitrate(txBps)));
        }
    } else {
        m_rateClock.start();
    }
    m_lastRx = rx;
    m_lastTx = tx;
    m_haveLast = true;
    m_totLabel->setText(QStringLiteral("Totale  ↓ %1   ↑ %2").arg(formatBytes(rx), formatBytes(tx)));
}

void MainWindow::appendLog(const QString& line) {
    // Difesa in profondità: anche se un backend dimenticasse di ripulire, nulla di segreto arriva al log.
    m_log->appendPlainText(QStringLiteral("%1  %2")
                               .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")),
                                    redactSecrets(line)));
}

// ---------------------------------------------------------------- Impostazioni

void MainWindow::loadSettings() {
    m_loading = true;
    QSettings s(QStringLiteral("PigeonVPN"), QStringLiteral("PigeonVPN"));

    m_nodes.clear();
    const QJsonArray arr = QJsonDocument::fromJson(s.value(QStringLiteral("nodes")).toString().toUtf8()).array();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        m_nodes.append({o[QLatin1String("name")].toString(), o[QLatin1String("country")].toString(),
                        o[QLatin1String("endpoint")].toString(), o[QLatin1String("publicKey")].toString()});
    }
    if (m_nodes.isEmpty()) m_nodes = defaultNodes();

    m_nodeCombo->clear();
    for (const ExitNode& n : std::as_const(m_nodes)) m_nodeCombo->addItem(n.label());
    m_nodeCombo->setCurrentIndex(qBound(0, s.value(QStringLiteral("nodeIndex"), 0).toInt(), int(m_nodes.size()) - 1));

    m_address->setText(s.value(QStringLiteral("address"), QStringLiteral("10.64.0.2/32, fd00:64::2/128")).toString());
    m_dns->setText(s.value(QStringLiteral("dns"), QStringLiteral("10.64.0.1")).toString());
    m_mtu->setValue(s.value(QStringLiteral("mtu"), 1420).toInt());
    m_killSwitch->setChecked(s.value(QStringLiteral("killSwitch"), false).toBool());
    m_dryRun->setChecked(s.value(QStringLiteral("dryRun"), true).toBool());
    loadNodeFields();
    m_loading = false;
}

void MainWindow::saveSettings() const {
    QSettings s(QStringLiteral("PigeonVPN"), QStringLiteral("PigeonVPN"));
    QJsonArray arr;
    for (const ExitNode& n : m_nodes)
        arr.append(QJsonObject{{"name", n.name}, {"country", n.country},
                               {"endpoint", n.endpoint}, {"publicKey", n.publicKey}});
    s.setValue(QStringLiteral("nodes"), QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    s.setValue(QStringLiteral("nodeIndex"), m_nodeCombo->currentIndex());
    s.setValue(QStringLiteral("address"), m_address->text());
    s.setValue(QStringLiteral("dns"), m_dns->text());
    s.setValue(QStringLiteral("mtu"), m_mtu->value());
    s.setValue(QStringLiteral("killSwitch"), m_killSwitch->isChecked());
    s.setValue(QStringLiteral("dryRun"), m_dryRun->isChecked());
    // PrivateKey / PresharedKey: volutamente NON salvate.
}

void MainWindow::loadNodeFields() {
    const int i = m_nodeCombo->currentIndex();
    if (i < 0 || i >= m_nodes.size()) return;
    m_endpoint->setText(m_nodes[i].endpoint);
    m_pubKey->setText(m_nodes[i].publicKey);
}
