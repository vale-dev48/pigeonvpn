# Sicurezza

- GUI mai root; solo `wg-quick up/down` elevato.
- Chiave privata: solo in memoria + `.conf` 0600 temporaneo (dir 0700), zeri + delete subito dopo `up`. `down` usa config senza chiavi (`m_downCfg`).
- Non salvata in QSettings. Log filtrati da `redactSecrets` (backend e `MainWindow::appendLog`).
- Validazione stretta di tutti i campi → niente injection nel `.conf`.
- Kill-switch: solo Linux, iptables/ip6tables in PostUp/PreDown (ricetta wg-quick(8)).
- Rischio residuo: file temp leggibile da root modificabile da stesso utente prima dell'`up` (finestra minima). Evoluzione: helper privilegiato con IPC.
