#include "SessionScreen.h"
#include "drivers/display/ScreenDriver.h"
#include <stdio.h>
#include "ui/ScreenUtils.h"

SessionScreen sessionScreen;

// Graph plot region (below the title rule, above the footer hint).
static constexpr int GRAPH_TOP = 16;
static constexpr int GRAPH_BASELINE = 54;
static constexpr int GRAPH_HEIGHT = GRAPH_BASELINE - GRAPH_TOP;

// Formats elapsed milliseconds as H:MM:SS (hours dropped once below an hour).
static void formatElapsed(uint32_t elapsedMs, char* out, int outSize) {
    uint32_t totalSeconds = elapsedMs / 1000;
    uint32_t hours = totalSeconds / 3600;
    uint32_t minutes = (totalSeconds % 3600) / 60;
    uint32_t seconds = totalSeconds % 60;
    if (hours > 0) {
        snprintf(out, outSize, "%lu:%02lu:%02lu", static_cast<unsigned long>(hours),
                 static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
    } else {
        snprintf(out, outSize, "%02lu:%02lu", static_cast<unsigned long>(minutes),
                 static_cast<unsigned long>(seconds));
    }
}

void SessionScreen::showStats(const SessionStats& stats) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    drawScreenHeader(*driver, "SESSION");

    driver->font(ScreenFont::Small);

    snprintf(buffer, sizeof(buffer), "Dist: %.2f km", stats.distanceKm);
    driver->text(0, 24, buffer);

    char elapsed[16];
    formatElapsed(stats.elapsedMs, elapsed, sizeof(elapsed));
    snprintf(buffer, sizeof(buffer), "Time: %s", elapsed);
    driver->text(0, 34, buffer);

    snprintf(buffer, sizeof(buffer), "Avg:  %.1f km/h", stats.avgSpeedKmh);
    driver->text(0, 44, buffer);

    snprintf(buffer, sizeof(buffer), "Max:  %.1f km/h", stats.maxSpeedKmh);
    driver->text(0, 54, buffer);

    driver->hline(0, 57, ScreenDriver::W);
    driver->font(ScreenFont::Tiny);
    driver->text(0, 63, "STR:graph THR:exit hold:reset");

    driver->flush();
}

void SessionScreen::showGraph(const SpeedHistory& history, float maxSpeedKmh) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[24];
    driver->clear();

    drawScreenHeader(*driver, "SPEED km/h");

    driver->hline(0, GRAPH_BASELINE, ScreenDriver::W);

    // Session max is >= every retained sample, so bars never overflow the region.
    const float scale = (maxSpeedKmh > 0.0f) ? maxSpeedKmh : 1.0f;
    for (int index = 0; index < history.count; index++) {
        float sample = speedHistoryAt(history, index);
        int barHeight = static_cast<int>((sample / scale) * GRAPH_HEIGHT + 0.5f);
        if (barHeight > 0) {
            driver->vline(index, GRAPH_BASELINE - barHeight, barHeight);
        }
    }

    driver->font(ScreenFont::Tiny);
    driver->text(0, 63, "THR:back");
    snprintf(buffer, sizeof(buffer), "max %.1f", maxSpeedKmh);
    driver->text(ScreenDriver::W - driver->textW(buffer), 63, buffer);

    driver->flush();
}
