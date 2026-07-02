#include "DebugScreen.h"
#include "drivers/display/ScreenDriver.h"
#include "ScreenUtils.h"
#include "config/ControlConfig.h"
#include <Arduino.h>
#include <stdio.h>

DebugScreen debugScreen;

void DebugScreen::showJoystickData(int joystickX, int joystickY) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    char buf[32];
    d->clear();

    d->font(ScreenFont::Small);
    d->text(0, 7, "DEBUG JOYSTICK TX");

    snprintf(buf, sizeof(buf), "X raw: %d", joystickX);
    d->text(0, 17, buf);

    snprintf(buf, sizeof(buf), "Y raw: %d", joystickY);
    d->text(0, 25, buf);

    snprintf(buf, sizeof(buf), "X delta: %+d", joystickX - JOYSTICK_CENTER_RAW);
    d->text(0, 35, buf);

    snprintf(buf, sizeof(buf), "Y delta: %+d", joystickY - JOYSTICK_CENTER_RAW);
    d->text(0, 43, buf);

    snprintf(buf, sizeof(buf), "Sent: %lu ms", millis());
    d->text(0, 51, buf);

    d->hline(0, 54, ScreenDriver::W);
    drawSysInfoFooter(*d, 62);

    d->flush();
}

void DebugScreen::showTelemetry(float batteryVoltage, int speedRpm) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    char buf[32];
    d->clear();

    d->font(ScreenFont::Small);
    d->text(0, 7, "DEBUG TELEMETRY RX");

    snprintf(buf, sizeof(buf), "Battery: %.2fV", batteryVoltage);
    d->text(0, 17, buf);

    snprintf(buf, sizeof(buf), "Speed: %d RPM", speedRpm);
    d->text(0, 25, buf);

    snprintf(buf, sizeof(buf), "RX: %lu ms", millis());
    d->text(0, 35, buf);

    d->text(0, 43, "Packet: new hash");

    d->hline(0, 47, ScreenDriver::W);
    drawSysInfoFooter(*d, 55);

    d->flush();
}
