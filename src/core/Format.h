#pragma once
#include <QString>
#include <QtGlobal>

inline QString formatBitrate(double bitsPerSec) {
    static const char* units[] = {"bit/s", "Kbit/s", "Mbit/s", "Gbit/s"};
    int u = 0;
    while (bitsPerSec >= 1000.0 && u < 3) { bitsPerSec /= 1000.0; ++u; }
    return QStringLiteral("%1 %2").arg(bitsPerSec, 0, 'f', u == 0 ? 0 : 1).arg(QLatin1String(units[u]));
}

inline QString formatBytes(quint64 bytes) {
    static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
    return QStringLiteral("%1 %2").arg(v, 0, 'f', u == 0 ? 0 : 1).arg(QLatin1String(units[u]));
}

inline QString formatUptime(qint64 seconds) {
    const qint64 h = seconds / 3600, m = (seconds / 60) % 60, s = seconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0')).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
}
