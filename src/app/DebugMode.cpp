#include "DebugMode.h"
#include "comm/TelemetryLink.h"
#include "comm/JoystickSender.h"
#include "config/ControlConfig.h"
#include "config/WifiConfig.h"
#include "ui/DebugScreen.h"
#include "ui/ModeSelectMenu.h"

DebugMode debugMode;

bool DebugMode::update(int joystickX, int joystickY) {
    bool newTelemetry = telemetryLink.process();
    if (newTelemetry) {
        TelemetryData latest = telemetryLink.getLatest();
        debugScreen.showTelemetry(latest.batteryVoltage, latest.speedRpm);
    } else {
        debugScreen.showJoystickData(joystickX, joystickY);
    }

    if (modeSelectMenu.checkOpenRequest()) {
        return true;
    }

    joystickSender.send(joystickX, joystickY, JOYSTICK_CENTER_RAW, RECEIVER_MAC);
    return false;
}
