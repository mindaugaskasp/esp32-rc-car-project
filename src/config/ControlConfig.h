#pragma once

// Joystick deadzone in ADC units (0-4095). Adjust to suppress natural drift.
#define JOY_DEADZONE 75

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

// Set to true if Y-up reverses the motor instead of driving forward.
// Detected by the Motor Dir calibration in the calibration menu.
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
// Stray noise pulses that survive the debounce are usually 1-2 per window;
// legitimate rotation produces many more. Raise if idle noise is still visible.
#define HALL_MIN_PULSES_FOR_RPM 3

// Set to true to launch the guided calibration menu on boot.
// Use Y-tap to navigate, Y-hold 1.5 s to select a calibration.
// Set back to false after calibration is complete.
#define CALIBRATION_MODE false
