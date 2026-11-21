#pragma once
#include <QString>

enum class VpnState { Disconnected, Connecting, Connected, Error };

inline QString toString(VpnState s) {
    switch (s) {
    case VpnState::Disconnected: return QStringLiteral("Disconnected");
    case VpnState::Connecting:   return QStringLiteral("Connecting");
    case VpnState::Connected:    return QStringLiteral("Connected");
    case VpnState::Error:        return QStringLiteral("Error");
    }
    return {};
}
