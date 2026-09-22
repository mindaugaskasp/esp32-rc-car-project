#pragma once
#include <stdint.h>
#include "app/SessionStatsLogic.h"

// Accumulates driving-session statistics (distance, time, average/max speed) and
// a rolling speed history for the graph. DashboardMode feeds it one call per
// telemetry frame; SessionMode reads the results. The session spans a whole power
// cycle — begin() is called once at startup.
class SessionTracker {
public:
    // Start (or restart) the session, clearing all accumulators.
    void begin();

    // Fold a fresh telemetry speed reading into the session. Computes the elapsed
    // slice from millis() since the previous call and integrates distance over it.
    void recordSpeed(int speedRpm);

    const SessionStats& stats() const { return _stats; }
    const SpeedHistory& history() const { return _history; }
    float currentSpeedKmh() const { return _currentSpeedKmh; }

private:
    // Telemetry gaps longer than this (menu navigation, a dropped link) are treated
    // as a pause boundary rather than driving time, so idle minutes don't inflate
    // distance/time or crater the average speed.
    static constexpr unsigned long MAX_SLICE_MS = 1000;
    // Cadence at which a point is appended to the speed-over-time graph.
    static constexpr unsigned long GRAPH_SAMPLE_MS = 500;

    SessionStats _stats{};
    SpeedHistory _history{};
    float _currentSpeedKmh = 0.0f;
    unsigned long _lastRecordMs = 0;
    unsigned long _lastGraphSampleMs = 0;
};

extern SessionTracker sessionTracker;
