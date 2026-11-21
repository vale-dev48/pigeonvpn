# Architettura

- `core/`: `TunnelConfig` (validate, `toWgQuick`, `redactedSummary`), `redactSecrets`, `VpnState`, formattatori.
- `vpn/`: `IVpnBackend` (connectTunnel/disconnectTunnel + segnali stateChanged/trafficTotals/logMessage).
  - `SimulatedBackend`: dry-run, timer, traffico random. Endpoint `error.test:*` → stato Error.
  - `WgQuickBackend`: QProcess asincrono, elevazione solo del comando wg-quick (pkexec / osascript / wireguard.exe).
- `ui/`: `MainWindow` (tab Tunnel + Configurazione, tray), `PigeonMascot` (QPainter, `paintPigeon` statico riusato per tray), `Sparkline`, `Backdrop` (alone animato), `Theme.h` (QSS).
- UI parla solo con `IVpnBackend`. Backend sostituibile a runtime solo da disconnesso.
- Scelta UI: Widgets + QSS (non QML) perché la mascotte è un QWidget.
- Build verificata: MinGW 13.1 + Qt 6.12.0 su Windows (Ninja).
