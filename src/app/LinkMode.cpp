#include "LinkMode.h"
#include "ui/LinkModeScreen.h"
#include "comm/LinkModeController.h"
#include "comm/DataTypes.h"
#include "config/WifiConfig.h"
#include "config/controller/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include "drivers/radio/EspNowDriver.h"
#include <Arduino.h>

LinkMode linkMode;

void LinkMode::begin() {
    _lastSendTime = 0;
    _wantsExit = false;
    // Require SW release before either button registers — the steering press that
    // selected this mode from the menu is likely still held on the first frame.
    _throttleSwWas = readButton(THROTTLE_SW_PIN);
    _steeringSwWas = readButton(STEERING_SW_PIN);
    redrawIfChanged(/*force=*/true);
}

void LinkMode::redrawIfChanged(bool force) {
    LinkPhyMode applied = linkModeController.applied();
    bool switching = linkModeController.switching();
    if (force || applied != _shownApplied || switching != _shownSwitching) {
        _shownApplied = applied;
        _shownSwitching = switching;
        linkModeScreen.show(applied, switching);
    }
}

void LinkMode::update() {
    unsigned long now = millis();

    // Send neutral continuously (not just on the loop heartbeat) so the handshake
    // has a steady stream of frames to ride and settles quickly. This mode never
    // drives the car.
    if (_lastSendTime == 0 || now - _lastSendTime >= SEND_INTERVAL_MS) {
        VehicleData neutral = makeNeutralCommand(static_cast<uint32_t>(now));
        sendData(neutral, RECEIVER_MAC);
        _lastSendTime = now;
    }

    bool steeringSw = readButton(STEERING_SW_PIN);
    if (steeringSw && !_steeringSwWas) {
        linkModeController.requestToggle();
    }
    _steeringSwWas = steeringSw;

    bool throttleSw = readButton(THROTTLE_SW_PIN);
    if (throttleSw && !_throttleSwWas) {
        _wantsExit = true;
    }
    _throttleSwWas = throttleSw;

    redrawIfChanged(/*force=*/false);
}

bool LinkMode::wantsExit() {
    bool result = _wantsExit;
    _wantsExit = false;
    return result;
}
