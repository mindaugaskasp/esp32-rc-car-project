#include "Screen.h"
#include "drivers/display/ScreenDriver.h"
#include "config/DebugConfig.h"
#include <Arduino.h>
#include <stdio.h>

Screen screen;

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

    // ── Battery row with latency centred between the two voltages ──────────
    driver->font(ScreenFont::Small);
    snprintf(buffer, sizeof(buffer), "CAR %.2fV", dashboard.carBatteryVoltage);
    driver->text(0, 10, buffer);

    snprintf(buffer, sizeof(buffer), "%.2fV RMT", dashboard.remoteBatteryVoltage);
    driver->text(ScreenDriver::W - driver->textW(buffer), 10, buffer);

    driver->font(ScreenFont::Tiny);
#if DEBUG_DASHBOARD_LINK_STATS
    // Rotate the single indicator slot between latency / loss / jitter so all
    // three fit without crowding the battery row.
    int statSlot = static_cast<int>((millis() / DASHBOARD_STAT_DWELL_MS) % 3);
    if (statSlot == 1 && dashboard.lossPercent >= 0) {
        snprintf(buffer, sizeof(buffer), "L:%d%%", dashboard.lossPercent);
    } else if (statSlot == 2 && dashboard.jitterMs >= 0) {
        snprintf(buffer, sizeof(buffer), "J:%dms", dashboard.jitterMs);
    } else if (dashboard.latencyMs >= 0) {
        snprintf(buffer, sizeof(buffer), "%dms", dashboard.latencyMs);
    } else {
        snprintf(buffer, sizeof(buffer), "--");
    }
#else
    if (dashboard.latencyMs < 0) snprintf(buffer, sizeof(buffer), "--");
    else snprintf(buffer, sizeof(buffer), "%dms", dashboard.latencyMs);
#endif
    driver->text((ScreenDriver::W - driver->textW(buffer)) / 2, 10, buffer);

    driver->hline(0, 14, ScreenDriver::W);

    // ── Menu hint (tiny, right-aligned in the gap above speed) ───────────────
    driver->font(ScreenFont::Tiny);
    driver->text(ScreenDriver::W - driver->textW("SW1:menu"), 20, "SW1:menu");

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
