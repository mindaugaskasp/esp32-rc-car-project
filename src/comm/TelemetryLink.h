#pragma once
#include <Arduino.h>
#include "comm/DataTypes.h"

// Owns the transmitter's inbound TelemetryData stream: ISR-safe receipt,
// change-detection dedup, and the RTT accumulators shared by the Dashboard's
// link-quality indicator and WiFi Ping mode.
class TelemetryLink {
public:
    // Registers the ESP-NOW receive callback. Call once after initEspNow().
    void begin();

    // Drains the pending ISR buffer. Returns true if new (changed) telemetry
    // was available this call — also logs it at DEBUG_LOG_MIN_INTERVAL_MS.
    bool process();

    TelemetryData getLatest() const { return _latest; }
    int getLatencyMs() const { return _latestLatencyMs; }
    uint32_t getReceivedCount() const { return _receivedCount; }

    struct RttDrainResult {
        uint32_t sampleCount;
        long sumMs;
        int minMs;
        int maxMs;
        int lastRtt;
    };
    // Snapshots and resets the RTT accumulators. Safe to call from any mode —
    // whichever mode last drained them gets exactly the samples since then,
    // so multiple replies arriving between two loop() ticks are never lost
    // the way a single "latest RTT" read would lose them.
    RttDrainResult drainRttStats();

    // ESP-NOW callback entry point. Public only because the C callback API
    // can't reach a private member — not part of the intended call surface.
    void handleReceive(const uint8_t* mac, const uint8_t* incomingData, int len);

private:
    static const int RTT_ACCUM_MIN_RESET = 1000000;
    static const int RTT_ACCUM_MAX_RESET = -1;

    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    TelemetryData _pending = {};
    volatile bool _pendingAvailable = false;
    volatile uint32_t _receivedCount = 0;

    volatile uint32_t _rttSampleCount = 0;
    volatile long _rttSumMs = 0;
    volatile int _rttMinMs = RTT_ACCUM_MIN_RESET;
    volatile int _rttMaxMs = RTT_ACCUM_MAX_RESET;

    TelemetryData _latest = {7.4f, 0};
    uint32_t _lastHash = 0;
    int _latestLatencyMs = -1;

    unsigned long _lastLogTime = 0;
};

extern TelemetryLink telemetryLink;
