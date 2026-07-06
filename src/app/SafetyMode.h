#pragma once
#include <stdint.h>

// Emergency-stop mode. While active it continuously transmits a safe-neutral
// command so the receiver holds the ESC at neutral, overriding any creep from a
// slightly-off stick rest point or a stuck/malfunctioning input. The joystick is
// ignored — this mode never drives the car. Mirrors WifiPingMode's
// begin()/update()/wantsExit() pattern.
class SafetyMode {
public:
    void begin();
    void update();
    // Returns true (once) when the user requests exit via throttle.
    bool wantsExit();

private:
    static const unsigned long SEND_INTERVAL_MS = 20; // ~50Hz neutral hold
    bool _throttleSwWas = false;
    unsigned long _lastSendTime = 0;
    bool _wantsExit = false;
};

extern SafetyMode safetyMode;
