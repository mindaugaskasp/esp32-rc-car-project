#pragma once
#include <stdint.h>

// Pure session-statistics accumulation — no Arduino dependency, so it is unit-
// testable natively (see test/test_session_stats_logic). The app-level
// SessionTracker owns the millis()/telemetry plumbing and folds each new speed
// sample in through these functions.

struct SessionStats {
    float distanceKm = 0.0f;
    uint32_t elapsedMs = 0; // connected driving time (sum of slices between telemetry samples)
    float maxSpeedKmh = 0.0f;
    float avgSpeedKmh = 0.0f; // distanceKm / elapsed hours
};

// One millisecond in hours — km/h * hours = km, and distance / hours = km/h.
static constexpr float MILLIS_PER_HOUR = 3600000.0f;

// Fold one time slice into the running stats: advance elapsed time by sliceMs and
// add the distance covered at speedKmh over that slice, then recompute the average.
inline void accumulateSession(SessionStats& stats, float speedKmh, uint32_t sliceMs) {
    if (speedKmh < 0.0f) speedKmh = 0.0f;
    stats.elapsedMs += sliceMs;
    stats.distanceKm += speedKmh * (sliceMs / MILLIS_PER_HOUR);
    if (speedKmh > stats.maxSpeedKmh) stats.maxSpeedKmh = speedKmh;
    if (stats.elapsedMs > 0) {
        stats.avgSpeedKmh = stats.distanceKm / (stats.elapsedMs / MILLIS_PER_HOUR);
    }
}

// Ring buffer of recent speed samples for the speed-over-time graph. One slot per
// plottable column of the 128px display; once full it scrolls (oldest overwritten).
static constexpr int SPEED_HISTORY_CAPACITY = 128;

struct SpeedHistory {
    float samples[SPEED_HISTORY_CAPACITY] = {};
    int count = 0; // valid samples, saturates at SPEED_HISTORY_CAPACITY
    int head = 0; // next write index (ring)
};

inline void pushSpeedSample(SpeedHistory& history, float speedKmh) {
    if (speedKmh < 0.0f) speedKmh = 0.0f;
    history.samples[history.head] = speedKmh;
    history.head = (history.head + 1) % SPEED_HISTORY_CAPACITY;
    if (history.count < SPEED_HISTORY_CAPACITY) history.count++;
}

// Reads the oldestIndex-th sample counting from the oldest still retained (0 =
// oldest, count-1 = newest), transparently handling the ring wrap.
inline float speedHistoryAt(const SpeedHistory& history, int oldestIndex) {
    const int oldest = (history.count == SPEED_HISTORY_CAPACITY) ? history.head : 0;
    return history.samples[(oldest + oldestIndex) % SPEED_HISTORY_CAPACITY];
}
