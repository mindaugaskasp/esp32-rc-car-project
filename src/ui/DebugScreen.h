#pragma once

class DebugScreen {
public:
    void showJoystickData(int joystickX, int joystickY);
    void showTelemetry(float batteryVoltage, int speedRpm);
};

extern DebugScreen debugScreen;
