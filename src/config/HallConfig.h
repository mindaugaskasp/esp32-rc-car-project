#pragma once

// Hall-effect speed sensor tuning (receiver). Pulse counting, RPM window, debounce.
// Kept as #define for native-test / Arduino parity — see CLAUDE.md.

// Hall effect sensor — pulses per full motor shaft revolution.
// When testing near the 4-pole brushless motor rotor: set to 2
//   (the rotor has 2 south-pole faces that trigger the 3144 per revolution).
// When mounted at the reduction gear with a single magnet: set to 1.
#define HALL_PULSES_PER_REV 2

// RPM is recalculated every this many milliseconds. Shorter = more responsive
// but noisier at low RPM (fewer pulses per window). 100–200 ms is a good range.
#define HALL_RPM_INTERVAL_MS 150

// ISR debounce: ignore any pulse arriving sooner than this after the previous
// one. At 30 000 RPM with 2 pulses/rev the minimum real interval is ~1 000 µs.
// Anything faster is ESC/motor switching noise on the signal wire.
#define HALL_MIN_PULSE_INTERVAL_US 1000

// Minimum pulses per interval window required to report a non-zero RPM from the
// count-based path. Below this the driver falls back to interval-based timing
// (rpmFromPulseInterval), so this gate no longer sets the minimum readable speed —
// it only marks where counting hands off to single-interval timing. Raise it if
// idle-noise blips leak into the count path.
#define HALL_MIN_PULSES_FOR_RPM 6

// No accepted pulse for this long -> the motor is stopped, report 0 immediately
// instead of holding the last reading until the next window. Must exceed one RPM
// window (HALL_RPM_INTERVAL_MS) so a merely slow window does not false-zero. It
// also bounds interval-based staleness: the slowest reported speed is
// 60e6 / (HALL_STOP_TIMEOUT_US * HALL_PULSES_PER_REV) RPM.
#define HALL_STOP_TIMEOUT_US 250000UL

// EMA smoothing of the reported RPM (new-sample weight = NUM/DEN). 1/2 gives a
// light filter (~one-window time constant at HALL_RPM_INTERVAL_MS) that damps
// window-to-window jitter without lagging real speed changes. Set DEN to 0 or 1
// with NUM==DEN to disable. On stop the reported value is forced to 0 (see
// updateHallSensor), so a dropout never leaves a stale reading to decay.
#define HALL_RPM_SMOOTHING_NUM 1
#define HALL_RPM_SMOOTHING_DEN 2
