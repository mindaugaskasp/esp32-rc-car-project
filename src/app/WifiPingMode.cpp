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
    _sw1Was = readButton(JOY1_SW_PIN);
    _sw2Was = readButton(JOY2_SW_PIN);

    telemetryLink.drainRttStats(); // discard anything accumulated before entering this mode
    _lastRxCount = telemetryLink.getRxCount();
}

void WifiPingMode::update() {
    unsigned long now = millis();

    // Send one ping per SEND_INTERVAL_MS slot
    if (_lastSendTime == 0 || now - _lastSendTime >= SEND_INTERVAL_MS) {
        VehicleData ping = makeNeutralCommand(static_cast<uint32_t>(now));
        sendData(ping, RECEIVER_MAC);
        _stats.sent++;
        _lastSendTime = now;
    }

    // Drain the telemetry pending buffer to prevent buildup
    telemetryLink.process();

    // Snapshot and drain the ISR-side RTT accumulators. These count every
    // incoming packet (bypassing the hash dedup in process()) and, critically,
    // never lose a sample even if several replies arrive between two loop()
    // iterations — reading only the single latest RTT here would silently
    // undercount and skew avg/min/max whenever replies coalesce like that.
    TelemetryLink::RttDrainResult drained = telemetryLink.drainRttStats();
    uint32_t currentRxCount = telemetryLink.getRxCount();

    if (currentRxCount != _lastRxCount) {
        _stats.received += (currentRxCount - _lastRxCount);
        _lastRxCount = currentRxCount;
    }

    if (drained.sampleCount > 0) {
        _rttSum += drained.sumMs;
        _rttSampleTotal += drained.sampleCount;
        _stats.avgRttMs = static_cast<int>(_rttSum / static_cast<long>(_rttSampleTotal));
        if (drained.minMs < _stats.minRtt) _stats.minRtt = drained.minMs;
        if (drained.maxMs > _stats.maxRtt) _stats.maxRtt = drained.maxMs;

        _stats.currentRtt = drained.lastRtt;
        if (_prevRtt >= 0)
            _stats.jitter = abs(drained.lastRtt - _prevRtt);
        _prevRtt = drained.lastRtt;
    }

    // Refresh display at 5Hz to avoid I2C overhead slowing the send rate
    if (_lastScreenUpdate == 0 || now - _lastScreenUpdate >= SCREEN_REFRESH_MS) {
        wifiPingScreen.show(_stats);
        _lastScreenUpdate = now;
    }

    bool sw1 = readButton(JOY1_SW_PIN);
    bool sw2 = readButton(JOY2_SW_PIN);

    if (sw2 && !_sw2Was) {
        _wantsExit = true;
    }
    if (sw1 && !_sw1Was) {
        begin();
    }

    _sw1Was = sw1;
    _sw2Was = sw2;
}

bool WifiPingMode::wantsExit() {
    bool result = _wantsExit;
    _wantsExit = false;
    return result;
}
