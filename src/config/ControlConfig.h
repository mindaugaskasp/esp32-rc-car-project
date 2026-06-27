#pragma once

// Per-axis joystick deadzone in ADC units (0-4095).
// X (steering): use a larger value — steering drift causes visible servo jitter.
// Y (throttle): smaller keeps throttle response quick from a standing start.
// Increase JOY_DEADZONE_X if the servo still twitches at rest.
#define JOY_DEADZONE_X 250
#define JOY_DEADZONE_Y 25

// Number of consecutive samples that must exceed the deadzone before sending.
#define JOY_CONSECUTIVE_THRESHOLD 3

// Joystick X axis calibration. Push the stick to each extreme and read the ADC
// value from the Serial log (logged as "joy x=<value>"), then set these to match.
// Default values are conservative — calibrating improves full-range servo travel.
#define JOYSTICK_X_MIN  100   // ADC at full left deflection
#define JOYSTICK_X_MAX  3950  // ADC at full right deflection

// Steering servo pulse range. Tune center trim if the wheels are not straight
// when the joystick is centered. Positive values move one direction, negative
// values move the other direction; try +/- 10 us steps.
#define SERVO_MIN_MICROS 500
#define SERVO_NEUTRAL_MICROS 1500
#define SERVO_MAX_MICROS 2500
#define SERVO_CENTER_TRIM_MICROS 80
#define SERVO_SMOOTHING_STEP_MICROS 150 // Increased for speed
// Minimum µs change required before writing a new position to the servo.
// Suppresses jitter from ADC noise (~5-20 ADC units → ~2-10 µs at mid-throw).
// Raise if jitter persists; lower if small steering corrections feel sluggish.
#define SERVO_DEADBAND_MICROS 8

// Minimum µs change before writing a new speed command to the ESC.
// ESC scale is ~0.2 µs/ADC unit so 5-20 unit noise → 1-4 µs; 4 µs kills it.
#define ESC_DEADBAND_MICROS 4

// Set to true if Y-up reverses the motor instead of driving forward.
// When enabled the ESC output is mirrored around neutral so Y-up = forward.
#define THROTTLE_INVERT false

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

