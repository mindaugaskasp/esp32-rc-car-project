#pragma once
#include <stdint.h>
#include "ui/WifiPingScreen.h"

// Sends a fixed-rate ping to the car and renders round-trip stats. Mirrors
// CalibrationFlow's begin()/update()/wantsExit() pattern.
class WifiPingMode {
public:
    // Resets all stats. Call both when entering this mode and to manually
    // reset the running stats (steering) while already in it.
    void begin();
    void update();
    // Returns true (once) when the user requests exit via throttle.
    bool wantsExit();

private:
    static constexpr int PING_RTT_UNSET = 9999; // sentinel for minRtt before any reply arrives
    static const unsigned long SEND_INTERVAL_MS = 20;
    static const unsigned long SCREEN_REFRESH_MS = 200; // 5Hz — avoid I2C overhead slowing the send rate

    PingStats _stats{};
    long _rttSum = 0;
    uint32_t _rttSampleTotal = 0; // valid-RTT samples folded into _rttSum so far
    int _prevRtt = -1;
    uint32_t _lastReceivedCount = 0;
    bool _throttleSwWas = false;
    bool _steeringSwWas = false;
    unsigned long _lastSendTime = 0;
    unsigned long _lastScreenUpdate = 0;
    bool _wantsExit = false;
};

extern WifiPingMode wifiPingMode;
