# PigeonVPN

Client VPN desktop WireGuard, C++17 + Qt 6 Widgets (QSS dark). Piccione disegnato con `QPainter`, modalità simulazione sicura.

## Build

Requisiti: CMake ≥ 3.21, compilatore C++17, Qt ≥ 6.4 (Widgets, Network).

### Linux
```bash
sudo apt install qt6-base-dev cmake ninja-build wireguard-tools policykit-1   # Debian/Ubuntu
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build && ./build/bin/PigeonVPN
```

### macOS
```bash
brew install qt cmake ninja wireguard-tools bash   # wg-quick richiede bash >= 4
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build && open build/bin/PigeonVPN.app
```

### Windows (MinGW di Qt)
```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.12.0\mingw_64\bin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build
windeployqt build\bin\PigeonVPN.exe
build\bin\PigeonVPN.exe
```
Modalità reale su Windows: installa WireGuard e avvia PigeonVPN **come amministratore**.

## Uso
- Default: **modalità simulazione** (tab Configurazione). Nessun privilegio, nessuna rete: handshake finto, traffico casuale, grafico e log.
- Per provare lo stato Errore in simulazione: endpoint del nodo = `error.test:51820`.
- Modalità reale: inserisci PrivateKey, Address, DNS, MTU e per ogni nodo Endpoint + PublicKey. `AllowedIPs = 0.0.0.0/0, ::/0` (full tunnel) è fisso.
- Chiudere la finestra manda l'app nel tray; "Esci" nel menu tray disconnette e chiude.

## Architettura
```
src/core  WireGuardConfig (validazione, render wg-quick, redazione segreti), VpnTypes, Format
src/vpn   IVpnBackend (interfaccia) ← SimulatedBackend | WgQuickBackend
src/ui    MainWindow, PigeonMascot (QPainter), Sparkline, Backdrop, Theme (QSS)
```

## Sicurezza / confine privilegi
- La GUI non è mai root. Solo `wg-quick up/down <file>` è elevato (`pkexec` Linux, `osascript` macOS, admin su Windows).
- La chiave privata esiste su disco solo nel `.conf` 0600 in una dir 0700, sovrascritto con zeri e cancellato appena il tunnel è su. Il `down` usa una config **senza chiavi**.
- Chiavi mai salvate in `QSettings`, mai nei log: `redactSecrets()` filtra ogni riga (backend + UI).
- Ogni campo è validato (chiavi base64 32 byte, CIDR, IP DNS, host:porta, MTU) prima del render: nessuna injection di direttive nel `.conf`.
- Limiti noti: kill-switch automatico solo Linux (iptables via `PostUp`/`PreDown`); statistiche su macOS/Windows richiedono `wg show` privilegiato. Un helper di sistema dedicato (servizio/daemon con IPC) sarebbe l'evoluzione per produzione.

## Licenza e contributi
MIT, vedi [LICENSE](LICENSE). Pull request e issue benvenute, in particolare: test su server WireGuard reali, kill-switch Windows/macOS, helper privilegiato con IPC.
Sito: https://vale-dev48.github.io/pigeonvpn/
