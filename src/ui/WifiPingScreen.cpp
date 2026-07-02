#include "WifiPingScreen.h"
#include "drivers/display/ScreenDriver.h"
#include <Arduino.h>
#include <stdio.h>

WifiPingScreen wifiPingScreen;

void WifiPingScreen::show(const PingStats& stats) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    char buf[32];
    d->clear();

    d->font(ScreenFont::Medium);
    d->text(0, 10, "WIFI PING");
    d->hline(0, 13, ScreenDriver::W);

    d->font(ScreenFont::Small);

    uint32_t lossPercent = (stats.sent > 0 && stats.sent >= stats.received)
        ? (stats.sent - stats.received) * 100 / stats.sent
        : 0;
    snprintf(buf, sizeof(buf), "Sent:%-5lu Loss:%2lu%%", (unsigned long)stats.sent, (unsigned long)lossPercent);
    d->text(0, 24, buf);

    if (stats.currentRtt >= 0) {
        snprintf(buf, sizeof(buf), "RTT:%4dms Avg:%4dms", stats.currentRtt, stats.avgRttMs);
    } else {
        snprintf(buf, sizeof(buf), "RTT:  ---  Avg:  ---");
    }
    d->text(0, 34, buf);

    if (stats.maxRtt >= 0) {
        snprintf(buf, sizeof(buf), "Min:%4dms Max:%4dms", stats.minRtt, stats.maxRtt);
    } else {
        snprintf(buf, sizeof(buf), "Min:  ---  Max:  ---");
    }
    d->text(0, 44, buf);

    snprintf(buf, sizeof(buf), "Jitter:%3dms", stats.jitter);
    d->text(0, 54, buf);

    d->hline(0, 57, ScreenDriver::W);
    d->font(ScreenFont::Tiny);
    d->text(0, 63, "SW1:exit  SW2:reset");

    d->flush();
}
