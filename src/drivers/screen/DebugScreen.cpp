#include "DebugScreen.h"
#include "ScreenDriver.h"

DebugScreen debugScreen;

void DebugScreen::enable(bool enabled) {
    this->enabled = enabled;
}

bool DebugScreen::isEnabled() const {
    return enabled;
}

void DebugScreen::showJoystickMoved(int joystickX, int joystickY) {
    if (!enabled) {
        return;
    }

    ScreenDriver* driver = getScreenDriver();
    if (driver) {
        driver->displayDebugJoystick(joystickX, joystickY);
    }
}

void DebugScreen::showTelemetryReceived(float batteryVoltage, int speedRpm) {
    if (!enabled) {
        return;
    }

    ScreenDriver* driver = getScreenDriver();
    if (driver) {
        driver->displayDebugTelemetry(batteryVoltage, speedRpm);
    }
}
