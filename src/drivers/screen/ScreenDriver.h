#pragma once

#include <stdint.h>

class ScreenDriver {
public:
    virtual ~ScreenDriver() = default;
    virtual void init() = 0;
    virtual void displayStartup(const char* message) = 0;
    virtual void displayConnectionEstablished() = 0;
    virtual void displayDashboard(float carBatteryVoltage, float remoteBatteryVoltage, int speedRpm, float speedKmh) = 0;
    virtual void displayTelemetry(float batteryVoltage, int speedRpm) = 0;
    virtual void displayDebugJoystick(int joystickX, int joystickY) = 0;
    virtual void displayDebugTelemetry(float batteryVoltage, int speedRpm) = 0;
    // Shared calibration display primitives used by all calibration screens.
    // barValue 0-4095 draws a progress bar; -1 omits it.
    virtual void displayCalibrationStep(const char* title, uint8_t step, uint8_t totalSteps, const char* instruction, int barValue) = 0;
    virtual void displayCalibrationResult(const char* title, const char* line1, const char* line2, const char* line3) = 0;
};

ScreenDriver* getScreenDriver();
