#include "WifiScanScreen.h"
#include "drivers/display/ScreenDriver.h"
#include <Arduino.h>
#include <stdio.h>

WifiScanScreen wifiScanScreen;

static const char SPINNER_CHARS[] = { '|', '/', '-', '\\' };
static const uint8_t SPINNER_COUNT = sizeof(SPINNER_CHARS);

void WifiScanScreen::showScanning() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    char buf[32];
    d->clear();

    d->font(ScreenFont::Medium);
    d->text(0, 10, "WIFI ANALYSIS");
    d->hline(0, 13, ScreenDriver::W);

    d->font(ScreenFont::Small);
    d->text(0, 28, "Scanning 2.4GHz...");
    snprintf(buf, sizeof(buf), "  [%c] Please wait", SPINNER_CHARS[_spinnerFrame % SPINNER_COUNT]);
    d->text(0, 40, buf);
    _spinnerFrame++;

    d->hline(0, 55, ScreenDriver::W);
    d->font(ScreenFont::Tiny);
    d->text(0, 63, "Checking all channels");

    d->flush();
}

void WifiScanScreen::showResult(const ChannelScanResult& result, int countdownSecs, bool broadcasting) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    char buf[32];
    d->clear();

    d->font(ScreenFont::Medium);
    d->text(0, 10, "WIFI ANALYSIS");
    d->hline(0, 13, ScreenDriver::W);

    d->font(ScreenFont::Small);

    snprintf(buf, sizeof(buf), "Channel:  %d", result.bestChannel);
    d->text(0, 24, buf);

    snprintf(buf, sizeof(buf), "Quality:  %s", result.qualityLabel);
    d->text(0, 34, buf);

    snprintf(buf, sizeof(buf), "APs seen: %d total", result.totalApCount);
    d->text(0, 44, buf);

    if (broadcasting) {
        snprintf(buf, sizeof(buf), ">Syncing car ch %d...", result.bestChannel);
    } else {
        snprintf(buf, sizeof(buf), "Ch load:  %d APs", result.channelApCount);
    }
    d->text(0, 54, buf);

    d->hline(0, 57, ScreenDriver::W);
    d->font(ScreenFont::Tiny);
    if (broadcasting) {
        snprintf(buf, sizeof(buf), "%ds  Bcasting  [SW:skip]", countdownSecs);
    } else {
        snprintf(buf, sizeof(buf), "%ds  [SW: skip]", countdownSecs);
    }
    d->text(0, 63, buf);

    d->flush();
}
