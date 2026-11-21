#pragma once
#include <QString>

struct ExitNode {
    QString name;       // es. "Frankfurt"
    QString country;    // codice ISO, es. "DE" (niente emoji-flag: dipendono dai font di sistema)
    QString endpoint;   // host:porta oppure [ipv6]:porta
    QString publicKey;  // chiave pubblica WireGuard del peer (base64)

    QString label() const { return country.isEmpty() ? name : QStringLiteral("%1  ·  %2").arg(name, country); }
};

struct TunnelConfig {
    QString privateKey;    // SEGRETO: mai loggare, mai salvare su disco in chiaro (tranne il file 0600 temporaneo)
    QString presharedKey;  // SEGRETO, opzionale
    QString address = QStringLiteral("10.64.0.2/32, fd00:64::2/128");
    QString dns = QStringLiteral("10.64.0.1");
    int mtu = 1420;
    bool killSwitch = false;
    ExitNode node;

    // Full tunnel: tutto il traffico IPv4 + IPv6 passa dal tunnel.
    static QString allowedIps() { return QStringLiteral("0.0.0.0/0, ::/0"); }

    // Stringa vuota = valida, altrimenti messaggio d'errore (mai contenente segreti).
    QString validate() const;

    // Genera il contenuto di un file wg-quick. CONTIENE LA CHIAVE PRIVATA: va scritto solo su file 0600.
    // Tutti i campi sono validati (niente newline/caratteri liberi) quindi non c'e' injection di direttive.
    QString toWgQuick(bool includeLinuxKillSwitch) const;

    // Riepilogo sicuro da loggare.
    QString redactedSummary() const;
};

bool isValidWgKey(const QString& base64);

// Rimuove da un testo qualsiasi chiave privata / preshared / token che somigliano a chiavi WireGuard.
QString redactSecrets(QString text);
