#include "WifiPingScreen.h"
#include "drivers/display/ScreenDriver.h"
#include <Arduino.h>
#include <stdio.h>

WifiPingScreen wifiPingScreen;

void WifiPingScreen::show(const PingStats& stats) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "WIFI PING");
    driver->hline(0, 13, ScreenDriver::W);

    driver->font(ScreenFont::Small);

    uint32_t lossPercent = (stats.sent > 0 && stats.sent >= stats.received)
        ? (stats.sent - stats.received) * 100 / stats.sent
        : 0;
    snprintf(buffer, sizeof(buffer), "Sent:%-5lu Loss:%2lu%%", static_cast<unsigned long>(stats.sent), static_cast<unsigned long>(lossPercent));
    driver->text(0, 24, buffer);

    if (stats.currentRtt >= 0) {
        snprintf(buffer, sizeof(buffer), "RTT:%4dms Avg:%4dms", stats.currentRtt, stats.avgRttMs);
    } else {
        snprintf(buffer, sizeof(buffer), "RTT:  ---  Avg:  ---");
    }
    driver->text(0, 34, buffer);

    if (stats.maxRtt >= 0) {
        snprintf(buffer, sizeof(buffer), "Min:%4dms Max:%4dms", stats.minRtt, stats.maxRtt);
    } else {
        snprintf(buffer, sizeof(buffer), "Min:  ---  Max:  ---");
    }
    driver->text(0, 44, buffer);

    snprintf(buffer, sizeof(buffer), "Jitter:%3dms", stats.jitter);
    driver->text(0, 54, buffer);

    driver->hline(0, 57, ScreenDriver::W);
    driver->font(ScreenFont::Tiny);
    driver->text(0, 63, "SW2:exit  SW1:reset");

    driver->flush();
}
