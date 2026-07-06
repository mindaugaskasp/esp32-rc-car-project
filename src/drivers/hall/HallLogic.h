#pragma once
#include "config/ControlConfig.h"
#include <stdint.h>

// Pure RPM computation for the hall speed sensor. No Arduino dependency, so every
// function here is unit-testable natively (see test/test_hall_logic).
//
// Two measurement regimes feed a single reported value:
//   - At speed (>= minPulses in a window) counting edges over the fixed window is
//     accurate and low-jitter — computeMotorRpm().
//   - At low speed the window holds too few edges to resolve, so we instead time a
//     single edge-to-edge interval — rpmFromPulseInterval(). This is what lets the
//     readout register slow rolling that the count-based gate would report as 0.
// selectWindowRpm() picks between them; smoothRpm() then filters the result.

// --- Count-based (high speed): edges accumulated over elapsedMs ---
//
//   RPM = (pulses / pulsesPerRev) / (elapsedMs / 60000)
//       = pulses * 60000 / (pulsesPerRev * elapsedMs)
//
// Returns 0 when:
//   - fewer than minPulses were seen in the window (idle-noise gate), or
//   - the inputs are degenerate (zero elapsed time, or non-positive pulsesPerRev)
//     — guards a divide-by-zero the caller's constants make unreachable today but a
//     future config change could introduce.
//
// Overflow note: pulses * 60000 stays within uint32_t because the ISR debounce
// (HALL_MIN_PULSE_INTERVAL_US) caps pulses at ~1 per ms, so a realistic window
// holds only a few hundred pulses (hundreds * 60000 << 2^32).
inline int computeMotorRpm(uint32_t pulseCount, unsigned long elapsedMs,
                           int pulsesPerRev = HALL_PULSES_PER_REV,
                           uint32_t minPulses = HALL_MIN_PULSES_FOR_RPM) {
    if (elapsedMs == 0 || pulsesPerRev <= 0) return 0;
    if (pulseCount < minPulses) return 0;
    return static_cast<int>(pulseCount * 60000UL / (static_cast<uint32_t>(pulsesPerRev) * static_cast<uint32_t>(elapsedMs)));
}

// --- Interval-based (low speed): time between two consecutive edges ---
//
// One edge-to-edge interval is one pulse period, so a full revolution takes
// intervalUs * pulsesPerRev microseconds:
//
//   RPM = 60e6 us/min / (intervalUs * pulsesPerRev)
//
// Resolution is per-pulse rather than per-window, so it reports far lower speeds
// than the count gate allows. Overflow: 60,000,000 < 2^32, and the denominator is
// only ever a few hundred thousand at most, so uint32_t is safe.
inline int rpmFromPulseInterval(uint32_t intervalUs, int pulsesPerRev = HALL_PULSES_PER_REV) {
    if (intervalUs == 0 || pulsesPerRev <= 0) return 0;
    return static_cast<int>(60000000UL / (intervalUs * static_cast<uint32_t>(pulsesPerRev)));
}

// Picks the best available reading for one window: count-based when enough edges
// arrived, otherwise the most recent inter-pulse interval — but only if that
// interval is fresh enough (<= maxIntervalUs) to reflect current motion rather
// than a pulse captured just before the motor stopped. Returns 0 when neither
// source is usable, which the driver's stop-timeout also enforces independently.
inline int selectWindowRpm(uint32_t pulseCount, unsigned long elapsedMs, uint32_t lastIntervalUs,
                           uint32_t maxIntervalUs,
                           int pulsesPerRev = HALL_PULSES_PER_REV,
                           uint32_t minPulses = HALL_MIN_PULSES_FOR_RPM) {
    int countBasedRpm = computeMotorRpm(pulseCount, elapsedMs, pulsesPerRev, minPulses);
    if (countBasedRpm > 0) return countBasedRpm;
    if (lastIntervalUs > 0 && lastIntervalUs <= maxIntervalUs) {
        return rpmFromPulseInterval(lastIntervalUs, pulsesPerRev);
    }
    return 0;
}

// Exponential moving average: smoothed = previous + (sample - previous) * num/den.
// weightNum/weightDen is the new-sample weight (alpha); a smaller ratio smooths
// harder. Rounds to nearest so a small step never gets stuck below one count, and
// signed arithmetic handles a falling sample. weightDen <= 0 disables filtering.
inline int smoothRpm(int previous, int sample,
                     int weightNum = HALL_RPM_SMOOTHING_NUM,
                     int weightDen = HALL_RPM_SMOOTHING_DEN) {
    if (weightDen <= 0) return sample;
    long scaledDelta = static_cast<long>(sample - previous) * weightNum;
    long roundingBias = (scaledDelta >= 0 ? weightDen : -weightDen) / 2;
    return previous + static_cast<int>((scaledDelta + roundingBias) / weightDen);
}
