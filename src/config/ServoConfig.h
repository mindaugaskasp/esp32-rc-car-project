#pragma once

// Steering-servo output tuning (PWM pulse widths, jitter suppression).
// Kept as #define for native-test / Arduino parity — see CLAUDE.md.

// Steering servo pulse range. Tune center trim if the wheels are not straight
// when the joystick is centered. Positive values move one direction, negative
// values move the other direction; try +/- 10 us steps.
#define SERVO_MIN_MICROS 500
#define SERVO_NEUTRAL_MICROS 1500
#define SERVO_MAX_MICROS 2500
#define SERVO_CENTER_TRIM_MICROS 0

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
