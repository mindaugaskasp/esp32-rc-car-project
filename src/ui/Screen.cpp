#include "Screen.h"
#include "drivers/display/ScreenDriver.h"
#include "config/DebugConfig.h"
#include <Arduino.h>
#include <stdio.h>

Screen screen;

// Low-battery blink: the affected voltage is hidden every other phase. Repaints
// ride on the telemetry cadence (~300ms heartbeat when idle), so the effective
// blink is approximate — that's fine for an attention cue.
static const unsigned long LOW_BATTERY_BLINK_MS = 500;

// Nested-arcs Wi-Fi glyph, 7px wide x 6px tall, built from rectangle primitives
// (the display driver exposes no per-pixel call, so 1x1 boxes stand in for pixels).
// yTop is the top row; the icon sits inside the battery row's baseline band.
static void drawWifiIcon(ScreenDriver* driver, int x, int yTop) {
    driver->hline(x + 1, yTop, 5);          // outer arc top
    driver->box(x, yTop + 1, 1, 1);         // outer arc left
    driver->box(x + 6, yTop + 1, 1, 1);     // outer arc right
    driver->hline(x + 2, yTop + 2, 3);      // inner arc top
    driver->box(x + 1, yTop + 3, 1, 1);     // inner arc left
    driver->box(x + 5, yTop + 3, 1, 1);     // inner arc right
    driver->box(x + 3, yTop + 5, 1, 1);     // signal dot
}

void Screen::begin() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    driver->init();
    showStartup("Initializing...");
}

void Screen::showStartup(const char* message) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    driver->clear();

    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "RC Remote");
    driver->hline(0, 13, ScreenDriver::W);

    driver->font(ScreenFont::Small);
    driver->text(0, 25, message);

    driver->flush();
}

void Screen::showConnectionEstablished() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    driver->clear();

    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "RC Remote");
    driver->hline(0, 13, ScreenDriver::W);

    driver->font(ScreenFont::Small);
    driver->text(0, 25, "Car connected!");
    driver->text(0, 35, "Loading dashboard...");

    driver->flush();
}

void Screen::showDashboard(const DashboardData& dashboard) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    // ── Battery row: car voltage left, remote voltage right ──────────────────
    bool lowBatteryBlinkVisible = (millis() / LOW_BATTERY_BLINK_MS) % 2 == 0;

    driver->font(ScreenFont::Small);
    snprintf(buffer, sizeof(buffer), "CAR %.2fV", dashboard.carBatteryVoltage);
    if (!dashboard.carBatteryLow || lowBatteryBlinkVisible) {
        driver->text(0, 10, buffer);
    }

    snprintf(buffer, sizeof(buffer), "%.2fV RMT", dashboard.remoteBatteryVoltage);
    if (!dashboard.remoteBatteryLow || lowBatteryBlinkVisible) {
        driver->text(ScreenDriver::W - driver->textW(buffer), 10, buffer);
    }

    driver->hline(0, 14, ScreenDriver::W);

    // ── Link stat + menu hint share the row below the divider ────────────────
    // The stat sits here rather than between the two voltages: at Small font the
    // battery row already spans ~119 of 128px, leaving no room to centre anything
    // between them without it colliding with a 2-digit voltage.
    driver->font(ScreenFont::Tiny);
    bool showWifiIcon = true;
    // In debug mode the single indicator slot rotates between latency / loss /
    // jitter so all three fit in one slot; otherwise it just shows latency. The
    // Wi-Fi icon only fronts the latency reading — loss/jitter are their own
    // labelled stats.
    int statSlot = dashboard.debugMode
        ? static_cast<int>((millis() / DASHBOARD_STAT_DWELL_MS) % 3)
        : 0;
    if (statSlot == 1 && dashboard.lossPercent >= 0) {
        snprintf(buffer, sizeof(buffer), "L:%d%%", dashboard.lossPercent);
        showWifiIcon = false;
    } else if (statSlot == 2 && dashboard.jitterMs >= 0) {
        snprintf(buffer, sizeof(buffer), "J:%dms", dashboard.jitterMs);
        showWifiIcon = false;
    } else if (dashboard.latencyMs >= 0) {
        snprintf(buffer, sizeof(buffer), "%dms", dashboard.latencyMs);
    } else {
        snprintf(buffer, sizeof(buffer), "--");
    }
    const int statBaseline = 20;
    if (showWifiIcon) {
        const int iconWidth = 7;
        const int iconGap = 2;
        const int iconTop = statBaseline - 6;
        drawWifiIcon(driver, 0, iconTop);
        driver->text(iconWidth + iconGap, statBaseline, buffer);
    } else {
        driver->text(0, statBaseline, buffer);
    }

    driver->text(ScreenDriver::W - driver->textW("STR:menu"), statBaseline, "STR:menu");

    // ── Speed ────────────────────────────────────────────────────────────────
    driver->font(ScreenFont::Large);
    snprintf(buffer, sizeof(buffer), "%.1f km/h", dashboard.speedKmh);
    driver->text((ScreenDriver::W - driver->textW(buffer)) / 2, 34, buffer);

    // ── Max speed ─────────────────────────────────────────────────────────────
    driver->font(ScreenFont::Small);
    snprintf(buffer, sizeof(buffer), "MAX %.1f", dashboard.maxSpeedKmh);
    driver->text((ScreenDriver::W - driver->textW(buffer)) / 2, 44, buffer);

    // ── RPM ───────────────────────────────────────────────────────────────────
    driver->font(ScreenFont::Medium);
    snprintf(buffer, sizeof(buffer), "%d RPM", dashboard.speedRpm);
    driver->text((ScreenDriver::W - driver->textW(buffer)) / 2, 57, buffer);

    // ── Debug badge — bottom-left, in the 6px band below the RPM row ────────────
    if (dashboard.debugMode) {
        driver->font(ScreenFont::Tiny);
        driver->text(0, ScreenDriver::H - 1, "DEBUG MODE");
    }

    // ── Low-battery badge — bottom-right, shares the band with the debug badge ──
    if (dashboard.carBatteryLow || dashboard.remoteBatteryLow) {
        driver->font(ScreenFont::Tiny);
        // Kept short so it clears the DEBUG MODE badge sharing this row (both are
        // Tiny; the long form spanned 68 of the 128px width).
        const char* lowBatteryLabel = dashboard.carBatteryLow && dashboard.remoteBatteryLow
            ? "LOW:CAR+RMT"
            : (dashboard.carBatteryLow ? "LOW:CAR" : "LOW:RMT");
        driver->text(ScreenDriver::W - driver->textW(lowBatteryLabel), ScreenDriver::H - 1, lowBatteryLabel);
    }

    driver->flush();
}

void Screen::showTelemetry(float batteryVoltage, int speedRpm) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char buffer[32];
    driver->clear();

    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "RC Car Telemetry");

    driver->font(ScreenFont::Small);
    snprintf(buffer, sizeof(buffer), "Battery: %.2fV", batteryVoltage);
    driver->text(0, 30, buffer);

    snprintf(buffer, sizeof(buffer), "Speed: %d RPM", speedRpm);
    driver->text(0, 45, buffer);

    driver->flush();
}
