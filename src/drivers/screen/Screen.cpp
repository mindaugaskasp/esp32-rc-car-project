#include "Screen.h"
#include "ScreenDriver.h"

Screen screen;

void Screen::begin() {
    ScreenDriver* driver = getScreenDriver();
    if (driver) {
        driver->init();
    }
}

void Screen::showStartup(const char* message) {
    ScreenDriver* driver = getScreenDriver();
    if (driver) {
        driver->displayStartup(message);
    }
}

void Screen::showConnectionEstablished() {
    ScreenDriver* driver = getScreenDriver();
    if (driver) {
        driver->displayConnectionEstablished();
    }
}

void Screen::showDashboard(float carBatteryVoltage, float remoteBatteryVoltage, int speedRpm, float speedKmh) {
    ScreenDriver* driver = getScreenDriver();
    if (driver) {
        driver->displayDashboard(carBatteryVoltage, remoteBatteryVoltage, speedRpm, speedKmh);
    }
}

void Screen::showTelemetry(float batteryVoltage, int speedRpm) {
    ScreenDriver* driver = getScreenDriver();
    if (driver) {
        driver->displayTelemetry(batteryVoltage, speedRpm);
    }
}
