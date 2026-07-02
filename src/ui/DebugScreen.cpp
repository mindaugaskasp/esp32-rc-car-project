#include "DebugScreen.h"
#include "drivers/display/ScreenDriver.h"
#include "ScreenUtils.h"
#include "config/ControlConfig.h"
#include <Arduino.h>
#include <stdio.h>

DebugScreen debugScreen;

void DebugScreen::showJoystickData(int joystickX, int joystickY) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    driver->font(ScreenFont::Small);
    driver->text(0, 7, "DEBUG JOYSTICK TX");

    snprintf(buffer, sizeof(buffer), "X raw: %d", joystickX);
    driver->text(0, 17, buffer);

    snprintf(buffer, sizeof(buffer), "Y raw: %d", joystickY);
    driver->text(0, 25, buffer);

    snprintf(buffer, sizeof(buffer), "X delta: %+d", joystickX - JOYSTICK_CENTER_RAW);
    driver->text(0, 35, buffer);

    snprintf(buffer, sizeof(buffer), "Y delta: %+d", joystickY - JOYSTICK_CENTER_RAW);
    driver->text(0, 43, buffer);

    snprintf(buffer, sizeof(buffer), "Sent: %lu ms", millis());
    driver->text(0, 51, buffer);

    driver->hline(0, 54, ScreenDriver::W);
    drawSysInfoFooter(*driver, 62);

    driver->flush();
}

void DebugScreen::showTelemetry(float batteryVoltage, int speedRpm) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    driver->font(ScreenFont::Small);
    driver->text(0, 7, "DEBUG TELEMETRY RX");

    snprintf(buffer, sizeof(buffer), "Battery: %.2fV", batteryVoltage);
    driver->text(0, 17, buffer);

    snprintf(buffer, sizeof(buffer), "Speed: %d RPM", speedRpm);
    driver->text(0, 25, buffer);

    snprintf(buffer, sizeof(buffer), "RX: %lu ms", millis());
    driver->text(0, 35, buffer);

    driver->text(0, 43, "Packet: new hash");

    driver->hline(0, 47, ScreenDriver::W);
    drawSysInfoFooter(*driver, 55);

    driver->flush();
}
