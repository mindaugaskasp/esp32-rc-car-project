#pragma once

#include "drivers/display/ScreenDriver.h"

struct DashboardData {
    float carBatteryVoltage;
    float remoteBatteryVoltage;
    int speedRpm;
    float speedKmh;
    float maxSpeedKmh;
    int latencyMs; // -1 = not yet measured
    int lossPercent; // -1 = not yet available
    int jitterMs; // -1 = not yet available
    bool debugMode; // true while the physical DEBUG switch is on — dashboard shows a DEBUG MODE badge
};

class Screen {
public:
    void begin();
    void showStartup(const char* message);
    void showConnectionEstablished();
    void showDashboard(const DashboardData& data);
    void showTelemetry(float batteryVoltage, int speedRpm);
};

extern Screen screen;
