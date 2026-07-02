#pragma once
#include <stdint.h>

struct PingStats {
    uint32_t sent;
    uint32_t received;
    int currentRtt; // ms; -1 until first reply
    int minRtt; // ms; 9999 until first reply
    int maxRtt; // ms; -1 until first reply
    int avgRttMs;
    int jitter; // abs(currentRtt - prevRtt), ms
};

class WifiPingScreen {
public:
    void show(const PingStats& stats);
};

extern WifiPingScreen wifiPingScreen;
