#pragma once

class DebugScreen {
public:
    // pageTag is a short "index/total" indicator (e.g. "1/2") drawn in the header.
    void showJoystickData(int joystickX, int joystickY, const char* pageTag);
    void showTelemetry(float batteryVoltage, int speedRpm, const char* pageTag);
    void showPacketTrace(bool enabled, const char* pageTag);
};

extern DebugScreen debugScreen;
