#include "DebugMode.h"
#include "comm/TelemetryLink.h"
#include "comm/JoystickSender.h"
#include "config/ControlConfig.h"
#include "config/controller/Esp32Pins.h"
#include "config/WifiConfig.h"
#include "drivers/controls/Controls.h"
#include "drivers/debug/PacketTrace.h"
#include "ui/DebugScreen.h"
#include "ui/ModeSelectMenu.h"
#include <Arduino.h>
#include <stdio.h>

DebugMode debugMode;

void DebugMode::begin() {
    _page = Page::Joystick;
    // Seed edge-detect to the current reading so a button still held from the
    // menu selection that entered this mode doesn't register on the first tick.
    _throttleSwWas = readButton(THROTTLE_SW_PIN);
    _steeringSwWas = readButton(STEERING_SW_PIN);
}

bool DebugMode::update(int joystickX, int joystickY) {
    telemetryLink.process(); // keep the telemetry cache fresh for the telemetry page

    // steering: a short press advances to the next page; a long hold
    // on the Packet Trace page toggles the runtime trace flag. The advance is
    // deferred to release so a long hold can be distinguished from a tap.
    bool steeringSw = readButton(STEERING_SW_PIN);
    if (steeringSw && !_steeringSwWas) {
        _steeringSwPressStart = millis();
        _steeringSwLongHandled = false;
    }
    if (steeringSw && !_steeringSwLongHandled && _page == Page::PacketTrace
            && millis() - _steeringSwPressStart >= STEERING_SW_LONG_PRESS_MS) {
        togglePacketTrace();
        _steeringSwLongHandled = true; // one toggle per hold; suppresses the page advance on release
    }
    if (!steeringSw && _steeringSwWas && !_steeringSwLongHandled) {
        _page = static_cast<Page>((static_cast<uint8_t>(_page) + 1) % PAGE_COUNT);
    }
    _steeringSwWas = steeringSw;

    char pageTag[8];
    snprintf(pageTag, sizeof(pageTag), "%u/%u",
             static_cast<unsigned>(_page) + 1, static_cast<unsigned>(PAGE_COUNT));

    switch (_page) {
        case Page::Joystick:
            debugScreen.showJoystickData(joystickX, joystickY, pageTag);
            break;
        case Page::Telemetry: {
            TelemetryData latest = telemetryLink.getLatest();
            debugScreen.showTelemetry(latest.batteryVoltage, latest.speedRpm, pageTag);
            break;
        }
        case Page::PacketTrace:
            debugScreen.showPacketTrace(isPacketTraceEnabled(), pageTag);
            break;
    }

    // throttle press: open the mode menu (back).
    bool throttleSw = readButton(THROTTLE_SW_PIN);
    if (throttleSw && !_throttleSwWas) {
        _throttleSwWas = throttleSw;
        return true;
    }
    _throttleSwWas = throttleSw;

    joystickSender.send(joystickX, joystickY, STEERING_CENTER_RAW, RECEIVER_MAC);
    return false;
}
