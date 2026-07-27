#include "TelemetryLink.h"
#include "config/DebugConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/debug/PacketTrace.h"
#include <esp_now.h>

TelemetryLink telemetryLink;

static uint32_t hashTelemetry(const TelemetryData& telemetry) {
    uint32_t voltagePart = static_cast<uint32_t>(telemetry.batteryVoltage * 100.0f + 0.5f);
    return (voltagePart << 16) | static_cast<uint32_t>(telemetry.speedRpm & 0xFFFF);
}

static void onTelemetryReceiveTrampoline(const uint8_t* mac, const uint8_t* incomingData, int len) {
    telemetryLink.handleReceive(mac, incomingData, len);
}

void TelemetryLink::begin() {
    esp_now_register_recv_cb(onTelemetryReceiveTrampoline);
}

void TelemetryLink::handleReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    (void)mac;
    if (len != sizeof(TelemetryData)) return;

    TelemetryData t;
    memcpy(&t, incomingData, sizeof(t));

    uint32_t now = millis();
    int rtt = (t.echoTimestampMs > 0 && now >= t.echoTimestampMs)
              ? static_cast<int>(now - t.echoTimestampMs)
              : -1;

    portENTER_CRITICAL(&_mux);
    _pending = t;
    _pendingAvailable = true;
    _receivedCount++;
    _remoteLinkModeRaw = t.linkMode;
    if (rtt >= 0) {
        _latestLatencyMs = rtt;
        _rttSampleCount++;
        _rttSumMs += rtt;
        if (rtt < _rttMinMs) _rttMinMs = rtt;
        if (rtt > _rttMaxMs) _rttMaxMs = rtt;
    }
    portEXIT_CRITICAL(&_mux);
}

bool TelemetryLink::process() {
    TelemetryData received;
    bool available;
    portENTER_CRITICAL(&_mux);
    available = _pendingAvailable;
    if (available) {
        received = _pending;
        _pendingAvailable = false;
    }
    portEXIT_CRITICAL(&_mux);

    if (!available) return false;

    uint32_t hash = hashTelemetry(received);
    if (hash == _lastHash) return false;

    _latest = received;
    _lastHash = hash;

    if (isPacketTraceEnabled()) {
        debugLogger.logf("[TRACE] RX telem %.2fV %d RPM echo=%lu",
                         received.batteryVoltage, received.speedRpm,
                         static_cast<unsigned long>(received.echoTimestampMs));
    }

#if DEBUG_LOG_MIN_INTERVAL_MS > 0
    unsigned long now = millis();
    if (now - _lastLogTime < DEBUG_LOG_MIN_INTERVAL_MS) {
        return true;
    }
    _lastLogTime = now;
#endif
    debugLogger.logf("Telemetry: %.2fV %d RPM", received.batteryVoltage, received.speedRpm);
    return true;
}

uint8_t TelemetryLink::getRemoteLinkModeRaw() const {
    portENTER_CRITICAL(const_cast<portMUX_TYPE*>(&_mux));
    uint8_t mode = _remoteLinkModeRaw;
    portEXIT_CRITICAL(const_cast<portMUX_TYPE*>(&_mux));
    return mode;
}

TelemetryLink::RttDrainResult TelemetryLink::drainRttStats() {
    RttDrainResult result;
    portENTER_CRITICAL(&_mux);
    result.sampleCount = _rttSampleCount;
    result.sumMs = _rttSumMs;
    result.minMs = _rttMinMs;
    result.maxMs = _rttMaxMs;
    result.lastRtt = _latestLatencyMs;
    _rttSampleCount = 0;
    _rttSumMs = 0;
    _rttMinMs = RTT_ACCUM_MIN_RESET;
    _rttMaxMs = RTT_ACCUM_MAX_RESET;
    portEXIT_CRITICAL(&_mux);
    return result;
}
