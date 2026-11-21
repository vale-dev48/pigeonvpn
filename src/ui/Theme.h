#pragma once
#include <QString>

// QSS dark-first. Le card "vetro" usano rgba semitrasparenti sopra il Backdrop.
// Gli stati (property "vpn") sono impostati da MainWindow e ripolish-ati.
inline QString appStyleSheet() {
    return QStringLiteral(R"QSS(
* { font-family: "Segoe UI", "Inter", "SF Pro Text", "Noto Sans", sans-serif; font-size: 13px; color: #e2e8f0; }

QToolTip { background: #0f172a; color: #e2e8f0; border: 1px solid #334155; padding: 4px 6px; }

QLabel#title    { font-size: 22px; font-weight: 700; color: #f8fafc; letter-spacing: 0.5px; }
QLabel#subtitle { color: #94a3b8; font-size: 12px; }
QLabel#badge    { background: rgba(251,191,36,0.14); color: #fbbf24; border: 1px solid rgba(251,191,36,0.4);
                  border-radius: 9px; padding: 2px 10px; font-size: 11px; font-weight: 600; }
QLabel#badge[sim="false"] { background: rgba(52,211,153,0.14); color: #34d399; border-color: rgba(52,211,153,0.4); }

QLabel#status { font-size: 18px; font-weight: 600; color: #f1f5f9; }
QLabel#status[vpn="connected"]    { color: #22d3ee; }
QLabel#status[vpn="connecting"]   { color: #fbbf24; }
QLabel#status[vpn="error"]        { color: #f87171; }
QLabel#status[vpn="disconnected"] { color: #cbd5e1; }
QLabel#detail { color: #94a3b8; }
QLabel#uptime { font-family: "Cascadia Mono", "Consolas", "Menlo", monospace; font-size: 24px; color: #f8fafc; }
QLabel#cardTitle { color: #94a3b8; font-size: 11px; font-weight: 600; letter-spacing: 1px; }
QLabel#rxValue { color: #22d3ee; font-size: 17px; font-weight: 600; }
QLabel#txValue { color: #a78bfa; font-size: 17px; font-weight: 600; }
QLabel#totValue { color: #e2e8f0; font-size: 12px; }
QLabel#ks { color: #94a3b8; }
QLabel#ks[on="true"] { color: #34d399; }

QFrame#card { background: rgba(30,41,59,0.55); border: 1px solid rgba(148,163,184,0.16); border-radius: 18px; }

QPushButton#connectBtn { min-height: 48px; border-radius: 24px; font-size: 15px; font-weight: 700; color: #04121a;
                         border: none; padding: 0 28px;
                         background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #22d3ee, stop:1 #818cf8); }
QPushButton#connectBtn:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #67e8f9, stop:1 #a5b4fc); }
QPushButton#connectBtn:pressed { background: #0891b2; }
QPushButton#connectBtn[vpn="connecting"] { color: #1c1200;
                         background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #fbbf24, stop:1 #fb923c); }
QPushButton#connectBtn[vpn="connected"] { color: #e2e8f0; background: rgba(51,65,85,0.9); border: 1px solid #475569; }
QPushButton#connectBtn[vpn="connected"]:hover { background: rgba(71,85,105,0.95); }
QPushButton#connectBtn[vpn="error"] { color: #fff; background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #ef4444, stop:1 #f97316); }
QPushButton#connectBtn:disabled { color: #64748b; background: rgba(51,65,85,0.5); }

QComboBox, QLineEdit, QSpinBox { background: rgba(15,23,42,0.75); border: 1px solid #334155; border-radius: 10px;
                                 padding: 7px 10px; selection-background-color: #0e7490; }
QComboBox:hover, QLineEdit:hover, QSpinBox:hover { border-color: #475569; }
QComboBox:focus, QLineEdit:focus, QSpinBox:focus { border-color: #22d3ee; }
QComboBox:disabled, QLineEdit:disabled, QSpinBox:disabled { color: #64748b; }
QComboBox::drop-down { border: none; width: 26px; }
QComboBox QAbstractItemView { background: #0f172a; border: 1px solid #334155; selection-background-color: #155e75; outline: 0; }

QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 6px; border: 1px solid #475569; background: rgba(15,23,42,0.75); }
QCheckBox::indicator:checked { background: #22d3ee; border-color: #22d3ee; }
QCheckBox::indicator:disabled { border-color: #334155; }

QPlainTextEdit#log { background: rgba(2,6,23,0.6); border: 1px solid rgba(148,163,184,0.14); border-radius: 12px;
                     font-family: "Cascadia Mono", "Consolas", "Menlo", monospace; font-size: 11px; color: #94a3b8; padding: 6px; }

QTabWidget::pane { border: none; background: transparent; }
QTabWidget::tab-bar { alignment: left; }
QTabBar::tab { background: transparent; color: #94a3b8; padding: 8px 18px; border-bottom: 2px solid transparent; font-weight: 600; }
QTabBar::tab:hover { color: #e2e8f0; }
QTabBar::tab:selected { color: #f8fafc; border-bottom: 2px solid #22d3ee; }

QMenu { background: #0f172a; border: 1px solid #334155; padding: 4px; }
QMenu::item { padding: 6px 22px; border-radius: 6px; }
QMenu::item:selected { background: #155e75; }

QScrollBar:vertical { background: transparent; width: 8px; }
QScrollBar::handle:vertical { background: #334155; border-radius: 4px; min-height: 24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)QSS");
}
