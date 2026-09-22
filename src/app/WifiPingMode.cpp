#include "WifiPingMode.h"
#include "comm/TelemetryLink.h"
#include "config/ControlConfig.h"
#include "config/WifiConfig.h"
#include "config/controller/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include "drivers/radio/EspNowDriver.h"
#include <Arduino.h>

WifiPingMode wifiPingMode;

void WifiPingMode::begin() {
    _stats = {};
    _stats.currentRtt = -1;
    _stats.minRtt = PING_RTT_UNSET;
    _stats.maxRtt = -1;
    _rttSum = 0;
    _rttSampleTotal = 0;
    _prevRtt = -1;
    _lastSendTime = 0;
    _lastScreenUpdate = 0;
    _wantsExit = false;
    // Require SW release before exit/reset registers — the button that selected
    // this mode from the menu is likely still held on the first frame.
    _throttleSwWas = readButton(THROTTLE_SW_PIN);
    _steeringSwWas = readButton(STEERING_SW_PIN);

    telemetryLink.drainRttStats(); // discard anything accumulated before entering this mode
    _lastReceivedCount = telemetryLink.getReceivedCount();
}

void WifiPingMode::sendPingIfDue(unsigned long now) {
    if (_lastSendTime != 0 && now - _lastSendTime < SEND_INTERVAL_MS) return;

    VehicleData ping = makeNeutralCommand(static_cast<uint32_t>(now));
    sendData(ping, RECEIVER_MAC);
    _stats.sent++;
    _lastSendTime = now;
}

// Snapshot and drain the ISR-side RTT accumulators. These count every incoming
// packet (bypassing the hash dedup in process()) and, critically, never lose a
// sample even if several replies arrive between two loop() iterations — reading
// only the single latest RTT here would silently undercount and skew avg/min/max
// whenever replies coalesce like that.
void WifiPingMode::accumulateRttStats() {
    const TelemetryLink::RttDrainResult drained = telemetryLink.drainRttStats();
    const uint32_t currentReceivedCount = telemetryLink.getReceivedCount();

    if (currentReceivedCount != _lastReceivedCount) {
        _stats.received += (currentReceivedCount - _lastReceivedCount);
        _lastReceivedCount = currentReceivedCount;
    }

    if (drained.sampleCount == 0) return;

    _rttSum += drained.sumMs;
    _rttSampleTotal += drained.sampleCount;
    _stats.avgRttMs = static_cast<int>(_rttSum / static_cast<long>(_rttSampleTotal));
    if (drained.minMs < _stats.minRtt) _stats.minRtt = drained.minMs;
    if (drained.maxMs > _stats.maxRtt) _stats.maxRtt = drained.maxMs;

    _stats.currentRtt = drained.lastRtt;
    if (_prevRtt >= 0) {
        _stats.jitter = abs(drained.lastRtt - _prevRtt);
    }
    _prevRtt = drained.lastRtt;
}

void WifiPingMode::refreshScreenIfDue(unsigned long now) {
    if (_lastScreenUpdate != 0 && now - _lastScreenUpdate < SCREEN_REFRESH_MS) return;
    wifiPingScreen.show(_stats);
    _lastScreenUpdate = now;
}

void WifiPingMode::handleButtons() {
    const bool throttleSw = readButton(THROTTLE_SW_PIN);
    const bool steeringSw = readButton(STEERING_SW_PIN);

    if (throttleSw && !_throttleSwWas) { // throttle = exit/back
        _wantsExit = true;
    }
    if (steeringSw && !_steeringSwWas) { // steering = reset stats (the in-mode action)
        begin();
    }

    _throttleSwWas = throttleSw;
    _steeringSwWas = steeringSw;
}

void WifiPingMode::update() {
    const unsigned long now = millis();

    sendPingIfDue(now);
    telemetryLink.process(); // drain the pending buffer to prevent buildup
    accumulateRttStats();
    refreshScreenIfDue(now);
    handleButtons();
}

bool WifiPingMode::wantsExit() {
    bool result = _wantsExit;
    _wantsExit = false;
    return result;
}
