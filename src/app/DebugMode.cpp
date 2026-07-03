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
    _sw1Was = readButton(JOY1_SW_PIN);
    _sw2Was = readButton(JOY2_SW_PIN);
}

bool DebugMode::update(int joystickX, int joystickY) {
    telemetryLink.process(); // keep the telemetry cache fresh for the telemetry page

    // SW2 (steering stick): a short press advances to the next page; a long hold
    // on the Packet Trace page toggles the runtime trace flag. The advance is
    // deferred to release so a long hold can be distinguished from a tap.
    bool sw2 = readButton(JOY2_SW_PIN);
    if (sw2 && !_sw2Was) {
        _sw2PressStart = millis();
        _sw2LongHandled = false;
    }
    if (sw2 && !_sw2LongHandled && _page == Page::PacketTrace
            && millis() - _sw2PressStart >= SW2_LONG_PRESS_MS) {
        togglePacketTrace();
        _sw2LongHandled = true; // one toggle per hold; suppresses the page advance on release
    }
    if (!sw2 && _sw2Was && !_sw2LongHandled) {
        _page = static_cast<Page>((static_cast<uint8_t>(_page) + 1) % PAGE_COUNT);
    }
    _sw2Was = sw2;

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

    // SW1 (throttle stick) press: open the mode menu.
    bool sw1 = readButton(JOY1_SW_PIN);
    if (sw1 && !_sw1Was) {
        _sw1Was = sw1;
        return true;
    }
    _sw1Was = sw1;

    joystickSender.send(joystickX, joystickY, JOYSTICK_CENTER_RAW, RECEIVER_MAC);
    return false;
}
