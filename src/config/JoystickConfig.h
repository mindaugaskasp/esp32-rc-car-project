#pragma once

// Joystick input calibration — raw ADC characteristics of both sticks (rest
// centers, deadzones, inversion, travel endpoints). Consumed by the servo/ESC
// maps, the send-decision logic, and the on-screen calibration.
//
// Kept as #define (not constexpr) so the same values compile in the native test
// environment and the Arduino build — see the note in CLAUDE.md.

// Steering (X) rest point in ADC units. Also the servo-center and steering-neutral
// reference. The physical stick does NOT rest at the nominal 2048 — set this from
// the joystick calibration ("JOYSTICK_CENTER_RAW" in the [JOY CAL] serial output).
#define JOYSTICK_CENTER_RAW 1870

// Per-axis joystick deadzone in ADC units (0-4095).
// X (steering): use a larger value — steering drift causes visible servo jitter.
// Y (throttle): smaller keeps throttle response quick from a standing start.
// Increase JOY_DEADZONE_X if the servo still twitches at rest.
#define JOY_DEADZONE_X 115
#define JOY_DEADZONE_Y 60

// Number of consecutive samples that must exceed the deadzone before sending.
#define JOY_CONSECUTIVE_THRESHOLD 3

// Full-scale 12-bit ADC reading, used as the pivot for axis inversion.
#define ADC_MAX_RAW 4095

// Nominal midpoint of a 12-bit ADC. A neutral display/bar reference and the pivot
// for on-screen gesture math — NOT the stick rest point (the physical sticks rest
// at JOYSTICK_CENTER_RAW / THROTTLE_CENTER_RAW, which differ from this nominal).
#define ADC_MIDPOINT_RAW 2048

// Y-stick gesture thresholds (raw ADC) for on-screen menu/calibration navigation:
// pushing above UP or below DOWN registers an up/down tap.
#define JOY_GESTURE_UP_RAW 3500
#define JOY_GESTURE_DOWN_RAW 500

// Per-axis inversion — set true when a stick is mounted so its travel runs
// opposite to the expected direction (depends on physical joystick orientation).
// Applied to the raw ADC reading before any deadzone or mapping, so steering /
// throttle AND menu up/down navigation all follow the same orientation.
// NOTE: this is the axis-level flip. THROTTLE_INVERT (EscConfig.h) only mirrors
// the ESC output — do not enable both for the throttle axis or they cancel out.
#define JOY_INVERT_X false
#define JOY_INVERT_Y true

// Steering (X) travel endpoints. Push the stick to each extreme and read the ADC
// value from the Serial log ("[JOY CAL] X ... travel[min..max]"), then set these.
// Default values are conservative — calibrating improves full-range servo travel.
#define JOYSTICK_X_MIN 0 // ADC at full left deflection
#define JOYSTICK_X_MAX 4095 // ADC at full right deflection

// Throttle (Y) calibration. The physical joystick does NOT rest at the nominal
// 2048 — measure the resting Y value from the "[JOY]" serial log with the stick
// released and set THROTTLE_CENTER_RAW to it. The ESC neutral deadzone is centered
// here, so an accurate value is what keeps the motor stopped at rest.
// THROTTLE_JOY_MIN/MAX are the full-deflection endpoints (default full ADC range);
// tighten them to the logged extremes if you want full ESC travel before the stops.
#define THROTTLE_CENTER_RAW 2252
#define THROTTLE_JOY_MIN 0
#define THROTTLE_JOY_MAX 4095
