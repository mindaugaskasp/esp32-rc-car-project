#pragma once
#include <stdint.h>
#include "comm/LinkModeLogic.h"

// Link Mode setting screen mode. Lets the user toggle the ESP-NOW radio PHY
// between Standard and Long Range from the remote: a steering-button press
// requests the switch, which the LinkModeController then negotiates with the car.
// Throttle exits. The mode itself only drives the UI and the toggle request — the
// actual handshake runs every loop tick in LinkModeController, independent of the
// active mode. Mirrors SafetyMode's begin()/update()/wantsExit() shape.
class LinkMode {
public:
    void begin();
    void update();
    bool wantsExit();

private:
    static constexpr unsigned long SEND_INTERVAL_MS = 20; // keep the link warm for snappy negotiation

    void redrawIfChanged(bool force);

    bool _throttleSwWas = false;
    bool _steeringSwWas = false;
    unsigned long _lastSendTime = 0;
    bool _wantsExit = false;

    LinkPhyMode _shownApplied = LinkPhyMode::Standard;
    bool _shownSwitching = false;
};

extern LinkMode linkMode;
