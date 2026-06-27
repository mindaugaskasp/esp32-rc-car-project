#include "Screen.h"
#include "ScreenDriver.h"
#include <stdio.h>

Screen screen;

void Screen::begin() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->init();
    showStartup("Initializing...");
}

void Screen::showStartup(const char* message) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->clear();

    d->font(ScreenFont::Medium);
    d->text(0, 10, "RC Remote");
    d->hline(0, 13, ScreenDriver::W);

    d->font(ScreenFont::Small);
    d->text(0, 25, message);

    d->flush();
}

void Screen::showConnectionEstablished() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->clear();

    d->font(ScreenFont::Medium);
    d->text(0, 10, "RC Remote");
    d->hline(0, 13, ScreenDriver::W);

    d->font(ScreenFont::Small);
    d->text(0, 25, "Car connected!");
    d->text(0, 35, "Loading dashboard...");

    d->flush();
}

void Screen::showDashboard(const DashboardData& dd) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    char buf[32];
    d->clear();

    // ── Battery row with latency centred between the two voltages ──────────
    d->font(ScreenFont::Small);
    snprintf(buf, sizeof(buf), "CAR %.2fV", dd.carBatteryVoltage);
    d->text(0, 10, buf);

    snprintf(buf, sizeof(buf), "%.2fV RMT", dd.remoteBatteryVoltage);
    d->text(ScreenDriver::W - d->textW(buf), 10, buf);

    d->font(ScreenFont::Tiny);
    if (dd.latencyMs < 0) snprintf(buf, sizeof(buf), "--");
    else                   snprintf(buf, sizeof(buf), "%dms", dd.latencyMs);
    d->text((ScreenDriver::W - d->textW(buf)) / 2, 10, buf);

    d->hline(0, 14, ScreenDriver::W);

    // ── Menu hint (tiny, right-aligned in the gap above speed) ───────────────
    d->font(ScreenFont::Tiny);
    d->text(ScreenDriver::W - d->textW("SW2:menu"), 20, "SW2:menu");

    // ── Speed ────────────────────────────────────────────────────────────────
    d->font(ScreenFont::Large);
    snprintf(buf, sizeof(buf), "%.1f km/h", dd.speedKmh);
    d->text((ScreenDriver::W - d->textW(buf)) / 2, 34, buf);

    // ── Max speed ─────────────────────────────────────────────────────────────
    d->font(ScreenFont::Small);
    snprintf(buf, sizeof(buf), "MAX %.1f", dd.maxSpeedKmh);
    d->text((ScreenDriver::W - d->textW(buf)) / 2, 44, buf);

    // ── RPM ───────────────────────────────────────────────────────────────────
    d->font(ScreenFont::Medium);
    snprintf(buf, sizeof(buf), "%d RPM", dd.speedRpm);
    d->text((ScreenDriver::W - d->textW(buf)) / 2, 57, buf);

    d->flush();
}

void Screen::showTelemetry(float batteryVoltage, int speedRpm) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    char buf[32];
    d->clear();

    d->font(ScreenFont::Medium);
    d->text(0, 10, "RC Car Telemetry");

    d->font(ScreenFont::Small);
    snprintf(buf, sizeof(buf), "Battery: %.2fV", batteryVoltage);
    d->text(0, 30, buf);

    snprintf(buf, sizeof(buf), "Speed: %d RPM", speedRpm);
    d->text(0, 45, buf);

    d->flush();
}
