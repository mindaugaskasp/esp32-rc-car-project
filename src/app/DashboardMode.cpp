#include "DashboardMode.h"
#include "app/SessionTracker.h"
#include "comm/TelemetryLink.h"
#include "comm/JoystickSender.h"
#include "drivers/controls/DebugModeSwitch.h"
#include "config/ControlConfig.h"
#include "config/WifiConfig.h"
#include "ui/Screen.h"
#include "drivers/display/ScreenDriver.h"
#include "drivers/hall/SpeedLogic.h"
#include "ui/ModeSelectMenu.h"
#include <Arduino.h>
#include <cmath>

DashboardMode dashboardMode;

void DashboardMode::markSetupComplete() {
    _setupCompletedAt = millis();
}

void DashboardMode::showSyncWarning() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    driver->clear();
    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "NO CONNECTION");
    driver->hline(0, 13, ScreenDriver::W);
    driver->font(ScreenFont::Small);
    driver->text(0, 25, "Channel sync failed.");
    driver->text(0, 35, "Restart car AND");
    driver->text(0, 45, "remote together to");
    driver->text(0, 55, "re-sync channels.");
    driver->flush();
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
    uint32_t receivedCount = telemetryLink.getReceivedCount();

    if (_lossWindowStart == 0) {
        _lossWindowStart = now;
        _lossWindowSentBase = sentCount;
        _lossWindowReceivedBase = receivedCount;
        return;
    }
    if (now - _lossWindowStart >= LOSS_WINDOW_MS) {
        uint32_t sentInWindow = sentCount - _lossWindowSentBase;
        uint32_t receivedInWindow = receivedCount - _lossWindowReceivedBase;
        if (sentInWindow > 0) {
            uint32_t lostInWindow = (receivedInWindow < sentInWindow) ? (sentInWindow - receivedInWindow) : 0;
            _lossPercent = static_cast<int>(lostInWindow * 100 / sentInWindow);
        }
        _lossWindowStart = now;
        _lossWindowSentBase = sentCount;
        _lossWindowReceivedBase = receivedCount;
    }
}

void DashboardMode::show() {
    TelemetryData latest = telemetryLink.getLatest();

    DashboardData dashboard;
    dashboard.carBatteryVoltage = latest.batteryVoltage;
    dashboard.remoteBatteryVoltage = MOCK_REMOTE_BATTERY_VOLTAGE;
    dashboard.speedRpm = latest.speedRpm;
    dashboard.speedKmh = motorRpmToKmh(latest.speedRpm);
    dashboard.maxSpeedKmh = sessionTracker.stats().maxSpeedKmh;
    dashboard.latencyMs = telemetryLink.getLatencyMs();
    dashboard.lossPercent = _lossPercent;
    dashboard.jitterMs = _jitterMs;
    dashboard.debugMode = isDebugModeActive();
    screen.showDashboard(dashboard);
}

bool DashboardMode::update(int joystickX, int joystickY) {
    unsigned long now = millis();

    if (!_connectionEstablished && _setupCompletedAt > 0
            && now - _setupCompletedAt > NO_CONNECTION_TIMEOUT_MS) {
        if (!_syncWarningShown) {
            showSyncWarning();
            _syncWarningShown = true;
        }
        joystickSender.send(joystickX, joystickY, STEERING_CENTER_RAW, RECEIVER_MAC);
        return false;
    }

    bool newTelemetry = telemetryLink.process();
    if (newTelemetry) {
        sessionTracker.recordSpeed(telemetryLink.getLatest().speedRpm);
        if (!_connectionEstablished) {
            _connectionEstablished = true;
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

    joystickSender.send(joystickX, joystickY, STEERING_CENTER_RAW, RECEIVER_MAC);
    return false;
}
