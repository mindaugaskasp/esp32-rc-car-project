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

// Set to true to launch the guided calibration menu on boot.
// Use Y-tap to navigate, Y-hold 1.5 s to select a calibration.
// Set back to false after calibration is complete.
#define CALIBRATION_MODE false
