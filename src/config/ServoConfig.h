#pragma once

// Steering-servo output tuning (PWM pulse widths, jitter suppression).
// Kept as #define for native-test / Arduino parity — see CLAUDE.md.

// Tune center trim if the wheels are not straight when the joystick is centered.
// Positive values move one direction, negative the other; try +/- 10 us steps.
#define SERVO_NEUTRAL_MICROS 1500
#define SERVO_CENTER_TRIM_MICROS 0
#define SERVO_CENTER_MICROS (SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS)

// Mechanical steering-travel limit. The servo is bolted to the car's steering
// linkage, which reaches its stops far short of the servo's own ~180 deg range;
// commanding past them stalls the servo against the rack and strips its gears.
// Raise only after confirming full stick still clears the stops.
#define SERVO_MAX_STEERING_DEGREES 15

// Pulse width per degree, x10 so the travel math stays exact in integers. 11.1
// us/deg is the standard hobby-servo scale (1000-2000 us spans 90 deg); measure
// the actual servo if full stick does not land where this predicts.
#define SERVO_MICROS_PER_DEGREE_X10 111
#define SERVO_MAX_TRAVEL_MICROS ((SERVO_MAX_STEERING_DEGREES * SERVO_MICROS_PER_DEGREE_X10) / 10)

// Derived from the trimmed center, so both lock stops stay inside
// SERVO_MAX_STEERING_DEGREES even when the center is trimmed off neutral.
#define SERVO_MIN_MICROS (SERVO_CENTER_MICROS - SERVO_MAX_TRAVEL_MICROS)
#define SERVO_MAX_MICROS (SERVO_CENTER_MICROS + SERVO_MAX_TRAVEL_MICROS)

// Minimum µs change required before writing a new position to the servo.
// Suppresses jitter from ADC noise (~5-20 ADC units → ~2-10 µs at mid-throw).
// Raise if jitter persists; lower if small steering corrections feel sluggish.
#define SERVO_DEADBAND_MICROS 8

// One-shot "link established" wiggle: a few small sweeps either side of center so
// the car physically signals it has synced with the transmitter. Runs once at
// connection time only, never in the control loop.
#define SERVO_TWITCH_OFFSET_MICROS 120
#define SERVO_TWITCH_COUNT 4
#define SERVO_TWITCH_HOLD_MS 120
