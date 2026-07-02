#pragma once
#include "config/ControlConfig.h"

static const int SERVO_JOYSTICK_CENTER_RAW = JOYSTICK_CENTER_RAW;

// Computes servo PWM pulse width (µs) for a raw 12-bit joystick X input.
// Matches Arduino map() / constrain() / abs() integer arithmetic — no Arduino.h needed.
// Applies deadzone, per-side mapping with calibration endpoints, center trim, and clamping.
inline int computeServoMicros(int rawX) {
    if (rawX < 0)    rawX = 0;
    if (rawX > 4095) rawX = 4095;

    const int center = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;

    int targetMicros = center;
    int diff = rawX - SERVO_JOYSTICK_CENTER_RAW;
    if (diff < 0) diff = -diff;

    if (diff > JOY_DEADZONE_X) {
        if (rawX < SERVO_JOYSTICK_CENTER_RAW) {
            // Equivalent to: map(rawX, JOYSTICK_X_MIN, SERVO_JOYSTICK_CENTER_RAW, SERVO_MIN_MICROS, center)
            targetMicros = (int)((long)(rawX - JOYSTICK_X_MIN) * (center - SERVO_MIN_MICROS)
                           / (SERVO_JOYSTICK_CENTER_RAW - JOYSTICK_X_MIN)) + SERVO_MIN_MICROS;
        } else {
            // Equivalent to: map(rawX, SERVO_JOYSTICK_CENTER_RAW, JOYSTICK_X_MAX, center, SERVO_MAX_MICROS)
            targetMicros = (int)((long)(rawX - SERVO_JOYSTICK_CENTER_RAW) * (SERVO_MAX_MICROS - center)
                           / (JOYSTICK_X_MAX - SERVO_JOYSTICK_CENTER_RAW)) + center;
        }
    }

    if (targetMicros < SERVO_MIN_MICROS) targetMicros = SERVO_MIN_MICROS;
    if (targetMicros > SERVO_MAX_MICROS) targetMicros = SERVO_MAX_MICROS;

    return targetMicros;
}
