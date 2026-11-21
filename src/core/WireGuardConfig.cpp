#include "core/WireGuardConfig.h"

#include <QByteArray>
#include <QHostAddress>
#include <QRegularExpression>
#include <QStringList>

bool isValidWgKey(const QString& s) {
    if (s.size() != 44 || !s.endsWith(QLatin1Char('='))) return false;
    const auto r = QByteArray::fromBase64Encoding(s.toLatin1(),
        QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    return r && r.decoded.size() == 32;
}

QString redactSecrets(QString t) {
    static const QRegularExpression kv(QStringLiteral(R"((PrivateKey|PresharedKey)\s*=\s*\S+)"),
                                       QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression b64(QStringLiteral(R"([A-Za-z0-9+/]{43}=)"));
    t.replace(kv, QStringLiteral("\\1 = <redatto>"));
    t.replace(b64, QStringLiteral("<chiave>"));
    return t;
}

namespace {

QStringList splitList(const QString& s) {
    QStringList out;
    for (const QString& part : s.split(QLatin1Char(','), Qt::SkipEmptyParts)) out << part.trimmed();
    return out;
}

bool validCidr(const QString& item) {
    static const QRegularExpression re(QStringLiteral(R"(^[0-9a-fA-F:.]+(/\d{1,3})?$)"));
    if (!re.match(item).hasMatch()) return false;
    const auto sub = QHostAddress::parseSubnet(item);
    return !sub.first.isNull();
}

bool validEndpoint(const QString& ep) {
    static const QRegularExpression re(
        QStringLiteral(R"(^(\[[0-9a-fA-F:.]+\]|[A-Za-z0-9]([A-Za-z0-9.\-]*[A-Za-z0-9])?):(\d{1,5})$)"));
    const auto m = re.match(ep);
    if (!m.hasMatch()) return false;
    const int port = m.captured(3).toInt();
    return port >= 1 && port <= 65535;
}

} // namespace

QString TunnelConfig::validate() const {
    if (!isValidWgKey(privateKey)) return QStringLiteral("PrivateKey mancante o non valida (base64, 32 byte).");
    if (!isValidWgKey(node.publicKey)) return QStringLiteral("PublicKey del nodo '%1' mancante o non valida.").arg(node.name);
    if (!presharedKey.isEmpty() && !isValidWgKey(presharedKey)) return QStringLiteral("PresharedKey non valida.");
    if (!validEndpoint(node.endpoint)) return QStringLiteral("Endpoint del nodo '%1' non valido (host:porta).").arg(node.name);
    if (mtu < 1280 || mtu > 1500) return QStringLiteral("MTU fuori range (1280-1500).");

    const QStringList addrs = splitList(address);
    if (addrs.isEmpty()) return QStringLiteral("Address mancante.");
    for (const QString& a : addrs)
        if (!validCidr(a)) return QStringLiteral("Address non valido: %1").arg(a);

    for (const QString& d : splitList(dns)) {
        QHostAddress h;
        if (!h.setAddress(d)) return QStringLiteral("DNS non valido: %1").arg(d);
    }
    return {};
}

QString TunnelConfig::toWgQuick(bool includeLinuxKillSwitch) const {
    QStringList l;
    l << QStringLiteral("[Interface]")
      << QStringLiteral("PrivateKey = %1").arg(privateKey)
      << QStringLiteral("Address = %1").arg(splitList(address).join(QStringLiteral(", ")));
    const QStringList dnsList = splitList(dns);
    if (!dnsList.isEmpty()) l << QStringLiteral("DNS = %1").arg(dnsList.join(QStringLiteral(", ")));
    l << QStringLiteral("MTU = %1").arg(mtu);

    if (includeLinuxKillSwitch && killSwitch) {
        // Kill-switch ufficiale da wg-quick(8): rifiuta ogni pacchetto che non esce dal tunnel / non e' marcato.
        const QString rule = QStringLiteral(
            "! -o %i -m mark ! --mark $(wg show %i fwmark) -m addrtype ! --dst-type LOCAL -j REJECT");
        l << QStringLiteral("PostUp = iptables -I OUTPUT %1 && ip6tables -I OUTPUT %1").arg(rule)
          << QStringLiteral("PreDown = iptables -D OUTPUT %1 && ip6tables -D OUTPUT %1").arg(rule);
    }

    l << QString() << QStringLiteral("[Peer]")
      << QStringLiteral("PublicKey = %1").arg(node.publicKey);
    if (!presharedKey.isEmpty()) l << QStringLiteral("PresharedKey = %1").arg(presharedKey);
    l << QStringLiteral("AllowedIPs = %1").arg(allowedIps())
      << QStringLiteral("Endpoint = %1").arg(node.endpoint)
      << QStringLiteral("PersistentKeepalive = 25")
      << QString();
    return l.join(QLatin1Char('\n'));
}

QString TunnelConfig::redactedSummary() const {
    return QStringLiteral("nodo=%1 endpoint=%2 mtu=%3 dns=%4 allowed=%5 psk=%6 killswitch=%7 privkey=<redatto>")
        .arg(node.name, node.endpoint).arg(mtu).arg(dns, allowedIps(),
             presharedKey.isEmpty() ? QStringLiteral("no") : QStringLiteral("si"),
             killSwitch ? QStringLiteral("si") : QStringLiteral("no"));
}
