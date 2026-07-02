#pragma once
#include "config/ControlConfig.h"
#include <stdint.h>

// Converts a hall-pulse count accumulated over elapsedMs into motor RPM.
// Pure integer arithmetic — no Arduino dependency, so it is unit-testable natively.
//
//   RPM = (pulses / pulsesPerRev) / (elapsedMs / 60000)
//       = pulses * 60000 / (pulsesPerRev * elapsedMs)
//
// Returns 0 when:
//   - fewer than minPulses were seen in the window (idle-noise gate), or
//   - the inputs are degenerate (zero elapsed time, or non-positive pulsesPerRev)
//     — guards against a divide-by-zero that the caller's constants make unreachable
//     today but a future config change could introduce.
//
// Overflow note: pulses * 60000 stays within uint32_t because the ISR debounce
// (HALL_MIN_PULSE_INTERVAL_US) caps pulses at ~1 per ms, so a realistic window
// holds only a few hundred pulses (hundreds * 60000 << 2^32).
inline int computeMotorRpm(uint32_t pulseCount, unsigned long elapsedMs,
                           int pulsesPerRev = HALL_PULSES_PER_REV,
                           uint32_t minPulses = HALL_MIN_PULSES_FOR_RPM) {
    if (elapsedMs == 0 || pulsesPerRev <= 0) return 0;
    if (pulseCount < minPulses) return 0;
    return (int)(pulseCount * 60000UL / ((uint32_t)pulsesPerRev * (uint32_t)elapsedMs));
}
