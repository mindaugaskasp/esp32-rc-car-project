#include "WifiScanScreen.h"
#include "drivers/display/ScreenDriver.h"
#include <Arduino.h>
#include <stdio.h>

WifiScanScreen wifiScanScreen;

static const char SPINNER_CHARS[] = { '|', '/', '-', '\\' };
static const uint8_t SPINNER_COUNT = sizeof(SPINNER_CHARS);

void WifiScanScreen::showScanning() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "WIFI ANALYSIS");
    driver->hline(0, 13, ScreenDriver::W);

    driver->font(ScreenFont::Small);
    driver->text(0, 28, "Scanning 2.4GHz...");
    snprintf(buffer, sizeof(buffer), "  [%c] Please wait", SPINNER_CHARS[_spinnerFrame % SPINNER_COUNT]);
    driver->text(0, 40, buffer);
    _spinnerFrame++;

    driver->hline(0, 55, ScreenDriver::W);
    driver->font(ScreenFont::Tiny);
    driver->text(0, 63, "Checking all channels");

    driver->flush();
}

void WifiScanScreen::showResult(const ChannelScanResult& result, int countdownSecs, bool broadcasting) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "WIFI ANALYSIS");
    driver->hline(0, 13, ScreenDriver::W);

    driver->font(ScreenFont::Small);

    snprintf(buffer, sizeof(buffer), "Channel:  %d", result.bestChannel);
    driver->text(0, 24, buffer);

    snprintf(buffer, sizeof(buffer), "Quality:  %s", result.qualityLabel);
    driver->text(0, 34, buffer);

    snprintf(buffer, sizeof(buffer), "APs seen: %d total", result.totalApCount);
    driver->text(0, 44, buffer);

    if (broadcasting) {
        // Live spinner: showResult() is called every ~200ms across the broadcast
        // window, so advancing the frame here gives a visibly turning indicator
        // while the car is being synced (the scan phase above is a single blocking
        // call and cannot animate).
        snprintf(buffer, sizeof(buffer), ">Syncing car ch %d [%c]",
                 result.bestChannel, SPINNER_CHARS[_spinnerFrame % SPINNER_COUNT]);
        _spinnerFrame++;
    } else {
        snprintf(buffer, sizeof(buffer), "Ch load:  %d APs", result.channelApCount);
    }
    driver->text(0, 54, buffer);

    driver->hline(0, 57, ScreenDriver::W);
    driver->font(ScreenFont::Tiny);
    if (broadcasting) {
        snprintf(buffer, sizeof(buffer), "%ds  Bcasting  [SW:skip]", countdownSecs);
    } else {
        snprintf(buffer, sizeof(buffer), "%ds  [SW: skip]", countdownSecs);
    }
    driver->text(0, 63, buffer);

    driver->flush();
}
