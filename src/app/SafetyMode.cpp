#include "SafetyMode.h"
#include "ui/SafetyScreen.h"
#include "comm/DataTypes.h"
#include "config/WifiConfig.h"
#include "config/controller/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include "drivers/radio/EspNowDriver.h"
#include <Arduino.h>

SafetyMode safetyMode;

void SafetyMode::begin() {
    _lastSendTime = 0;
    _wantsExit = false;
    // Require SW release before exit registers — the steering press that selected this
    // mode from the menu is likely still held on the first frame.
    _throttleSwWas = readButton(THROTTLE_SW_PIN);
    safetyScreen.show();
}

void SafetyMode::update() {
    unsigned long now = millis();

    // Force the ESC to neutral every tick. Sending continuously (not just on the
    // heartbeat cadence) cancels creep immediately and keeps the receiver's
    // packet-loss watchdog from tripping while the stop is engaged.
    if (_lastSendTime == 0 || now - _lastSendTime >= SEND_INTERVAL_MS) {
        VehicleData stop = makeNeutralCommand(static_cast<uint32_t>(now));
        sendData(stop, RECEIVER_MAC);
        _lastSendTime = now;
    }

    bool throttleSw = readButton(THROTTLE_SW_PIN);
    if (throttleSw && !_throttleSwWas) {
        _wantsExit = true;
    }
    _throttleSwWas = throttleSw;
}

bool SafetyMode::wantsExit() {
    bool result = _wantsExit;
    _wantsExit = false;
    return result;
}
