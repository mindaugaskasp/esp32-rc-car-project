#pragma once

class DebugScreen {
public:
    void enable(bool enabled);
    bool isEnabled() const;
    void showJoystickMoved(int joystickX, int joystickY);
    void showTelemetryReceived(float batteryVoltage, int speedRpm);

private:
    bool enabled = false;
};

extern DebugScreen debugScreen;
