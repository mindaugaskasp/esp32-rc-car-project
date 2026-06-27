#pragma once

#include "drivers/screen/ScreenDriver.h"

class Sh1106ScreenDriver : public ScreenDriver {
public:
    void init() override;
    void displayStartup(const char* message) override;
    void displayConnectionEstablished() override;
    void displayDashboard(float carBatteryVoltage, float remoteBatteryVoltage, int speedRpm, float speedKmh) override;
    void displayTelemetry(float batteryVoltage, int speedRpm) override;
    void displayDebugJoystick(int joystickX, int joystickY) override;
    void displayDebugTelemetry(float batteryVoltage, int speedRpm) override;
    void displayCalibrationStep(const char* title, uint8_t step, uint8_t totalSteps, const char* instruction, int barValue) override;
    void displayCalibrationResult(const char* title, const char* line1, const char* line2, const char* line3) override;
};
