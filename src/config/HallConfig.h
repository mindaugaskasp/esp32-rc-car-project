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

// Minimum pulses per interval window required to report a non-zero RPM.
// Raise this if idle noise blips are still visible. At the reduction gear
// with 1 pulse/rev, 5 pulses/window = 2000 RPM minimum readable speed,
// which is well below any real driving speed at the wheel.
#define HALL_MIN_PULSES_FOR_RPM 6
