#include "DashboardMode.h"
#include "comm/TelemetryLink.h"
#include "comm/JoystickSender.h"
#include "config/ControlConfig.h"
#include "config/WifiConfig.h"
#include "ui/Screen.h"
#include "drivers/display/ScreenDriver.h"
#include "ui/ModeSelectMenu.h"
#include <Arduino.h>
#include <cmath>

DashboardMode dashboardMode;

static float speedKmh(int rpm) { return rpm * 0.05f; }

void DashboardMode::markSetupComplete() {
    _setupCompletedAt = millis();
}

void DashboardMode::showSyncWarning() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->clear();
    d->font(ScreenFont::Medium);
    d->text(0, 10, "NO CONNECTION");
    d->hline(0, 13, ScreenDriver::W);
    d->font(ScreenFont::Small);
    d->text(0, 25, "Channel sync failed.");
    d->text(0, 35, "Restart car AND");
    d->text(0, 45, "remote together to");
    d->text(0, 55, "re-sync channels.");
    d->flush();
}

void DashboardMode::updateLinkStats(bool newTelemetry) {
    if (newTelemetry) {
        int latencyMs = telemetryLink.getLatencyMs();
        if (latencyMs >= 0) {
            if (_prevRtt >= 0) _jitterMs = abs(latencyMs - _prevRtt);
            _prevRtt = latencyMs;
        }
    }

    unsigned long now = millis();
    uint32_t sentCount = joystickSender.getSentCount();
    uint32_t rxCount   = telemetryLink.getRxCount();

    if (_lossWindowStart == 0) {
        _lossWindowStart    = now;
        _lossWindowSentBase = sentCount;
        _lossWindowRxBase   = rxCount;
        return;
    }
    if (now - _lossWindowStart >= LOSS_WINDOW_MS) {
        uint32_t sentInWindow = sentCount - _lossWindowSentBase;
        uint32_t rxInWindow   = rxCount - _lossWindowRxBase;
        if (sentInWindow > 0) {
            uint32_t lostInWindow = (rxInWindow < sentInWindow) ? (sentInWindow - rxInWindow) : 0;
            _lossPercent = (int)(lostInWindow * 100 / sentInWindow);
        }
        _lossWindowStart    = now;
        _lossWindowSentBase = sentCount;
        _lossWindowRxBase   = rxCount;
    }
}

void DashboardMode::show() {
    TelemetryData latest = telemetryLink.getLatest();
    float kmh = speedKmh(latest.speedRpm);
    if (kmh > _maxSpeedKmh) _maxSpeedKmh = kmh;

    DashboardData d;
    d.carBatteryVoltage    = latest.batteryVoltage;
    d.remoteBatteryVoltage = MOCK_REMOTE_BATTERY_VOLTAGE;
    d.speedRpm             = latest.speedRpm;
    d.speedKmh             = kmh;
    d.maxSpeedKmh          = _maxSpeedKmh;
    d.latencyMs            = telemetryLink.getLatencyMs();
    d.lossPercent          = _lossPercent;
    d.jitterMs             = _jitterMs;
    screen.showDashboard(d);
}

bool DashboardMode::update(int joystickX, int joystickY) {
    unsigned long now = millis();

    if (!_connectionEstablished && _setupCompletedAt > 0
            && now - _setupCompletedAt > NO_CONNECTION_TIMEOUT_MS) {
        if (!_syncWarningShown) {
            showSyncWarning();
            _syncWarningShown = true;
        }
        joystickSender.send(joystickX, joystickY, JOYSTICK_CENTER_RAW, RECEIVER_MAC);
        return false;
    }

    bool newTelemetry = telemetryLink.process();
    if (newTelemetry) {
        if (!_connectionEstablished) {
            _connectionEstablished   = true;
            _connectionEstablishedAt = now;
        }
        if (!_dashboardShown) {
            screen.showConnectionEstablished();
        } else {
            show();
        }
    }
    if (_connectionEstablished && !_dashboardShown
            && now - _connectionEstablishedAt >= CONNECTION_MESSAGE_MS) {
        _dashboardShown = true;
        show();
    }

    updateLinkStats(newTelemetry);

    if (modeSelectMenu.checkOpenRequest()) {
        return true;
    }

    joystickSender.send(joystickX, joystickY, JOYSTICK_CENTER_RAW, RECEIVER_MAC);
    return false;
}
