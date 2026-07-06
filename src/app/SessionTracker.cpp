#include "SessionTracker.h"
#include "drivers/hall/SpeedLogic.h"
#include <Arduino.h>

SessionTracker sessionTracker;

void SessionTracker::begin() {
    _stats = {};
    _history = {};
    _currentSpeedKmh = 0.0f;
    _lastRecordMs = 0;
    _lastGraphSampleMs = 0;
}

void SessionTracker::recordSpeed(int speedRpm) {
    unsigned long now = millis();
    float speedKmh = motorRpmToKmh(speedRpm);
    _currentSpeedKmh = speedKmh;

    if (_lastRecordMs != 0) {
        unsigned long sliceMs = now - _lastRecordMs;
        // Skip (don't count) gaps longer than MAX_SLICE_MS — a resume after a pause,
        // not continuous driving.
        if (sliceMs <= MAX_SLICE_MS) {
            accumulateSession(_stats, speedKmh, static_cast<uint32_t>(sliceMs));
        }
    }
    _lastRecordMs = now;

    if (_lastGraphSampleMs == 0 || now - _lastGraphSampleMs >= GRAPH_SAMPLE_MS) {
        pushSpeedSample(_history, speedKmh);
        _lastGraphSampleMs = now;
    }
}
