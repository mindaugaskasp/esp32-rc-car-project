#pragma once

class Screen {
public:
    void begin();
    void showStartup(const char* message);
    void showConnectionEstablished();
    void showDashboard(float carBatteryVoltage, float remoteBatteryVoltage, int speedRpm, float speedKmh);
    void showTelemetry(float batteryVoltage, int speedRpm);
};

extern Screen screen;
