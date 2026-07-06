#include "DebugScreen.h"
#include "drivers/display/ScreenDriver.h"
#include "ScreenUtils.h"
#include "config/ControlConfig.h"
#include <Arduino.h>
#include <stdio.h>

DebugScreen debugScreen;

// Title (small font) with a right-aligned "index/total" page tag. Shared by both
// debug pages so only the middle rows differ between them.
static void drawDebugHeader(ScreenDriver& driver, const char* title, const char* pageTag) {
    driver.font(ScreenFont::Small);
    driver.text(0, 7, title);
    driver.font(ScreenFont::Tiny);
    driver.text(ScreenDriver::W - driver.textW(pageTag), 7, pageTag);
}

// Divider, SW hint, and system-info footer shared by the debug pages. swHint
// differs per page (page-cycling vs. the trace page's hold-to-toggle gesture).
static void drawDebugFooter(ScreenDriver& driver, const char* swHint) {
    driver.hline(0, 48, ScreenDriver::W);
    driver.font(ScreenFont::Tiny);
    driver.text(0, 55, swHint);
    drawSysInfoFooter(driver, 63);
}

void DebugScreen::showJoystickData(int joystickX, int joystickY, const char* pageTag) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    drawDebugHeader(*driver, "DEBUG JOYSTICK TX", pageTag);

    driver->font(ScreenFont::Small);
    snprintf(buffer, sizeof(buffer), "X raw: %d", joystickX);
    driver->text(0, 17, buffer);

    snprintf(buffer, sizeof(buffer), "Y raw: %d", joystickY);
    driver->text(0, 26, buffer);

    snprintf(buffer, sizeof(buffer), "X delta: %+d", joystickX - STEERING_CENTER_RAW);
    driver->text(0, 36, buffer);

    snprintf(buffer, sizeof(buffer), "Y delta: %+d", joystickY - THROTTLE_CENTER_RAW);
    driver->text(0, 44, buffer);

    drawDebugFooter(*driver, "THR:menu  STR:next page");
    driver->flush();
}

void DebugScreen::showTelemetry(float batteryVoltage, int speedRpm, const char* pageTag) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    drawDebugHeader(*driver, "DEBUG TELEMETRY RX", pageTag);

    driver->font(ScreenFont::Small);
    snprintf(buffer, sizeof(buffer), "Battery: %.2fV", batteryVoltage);
    driver->text(0, 17, buffer);

    snprintf(buffer, sizeof(buffer), "Speed: %d RPM", speedRpm);
    driver->text(0, 26, buffer);

    snprintf(buffer, sizeof(buffer), "RX: %lu ms", millis());
    driver->text(0, 36, buffer);

    drawDebugFooter(*driver, "THR:menu  STR:next page");
    driver->flush();
}

void DebugScreen::showPacketTrace(bool enabled, const char* pageTag) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    driver->clear();

    drawDebugHeader(*driver, "DEBUG PACKET TRACE", pageTag);

    driver->font(ScreenFont::Small);
    driver->text(0, 20, "Serial trace:");
    driver->text(0, 32, enabled ? "  ON" : "  OFF");

    driver->font(ScreenFont::Tiny);
    driver->text(0, 44, "Heavy UART load - off");

    drawDebugFooter(*driver, "THR:menu  STR hold:toggle");
    driver->flush();
}
