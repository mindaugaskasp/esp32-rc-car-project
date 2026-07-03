#pragma once
#include "config/ControlConfig.h"

// ESC PWM pulse width constants (microseconds)
static const int ESC_MIN_MICROS = 1100;
static const int ESC_NEUTRAL_MICROS = 1500;
static const int ESC_MAX_MICROS = 1900;

// Servo-library attach() bounds. Deliberately wider than the operational
// ESC_MIN/MAX range so the driver can command the full standard 1000-2000µs span
// during ESC arming/programming without the library clamping it to the run range.
static const int ESC_ATTACH_MIN_MICROS = 1000;
static const int ESC_ATTACH_MAX_MICROS = 2000;

// Computes the ESC PWM pulse width (µs) for a raw 12-bit joystick Y input.
// Center-based, mirroring computeServoMicros: a symmetric deadzone (JOY_DEADZONE_Y)
// around the measured throttle rest point (THROTTLE_CENTER_RAW) forces neutral, and
// each side maps independently — below center to reverse/brake, above center to
// forward. Centering on the real rest value is what keeps the motor stopped at rest
// even when the stick does not sit at the nominal 2048.
// Matches Arduino map() / constrain() / abs() integer arithmetic — no Arduino.h needed.
// Inputs outside [0, 4095] are clamped.
// When THROTTLE_INVERT is true the output is mirrored around neutral (Y-up = forward).
inline int computeEscMicros(int rawY) {
    if (rawY < 0) rawY = 0;
    if (rawY > 4095) rawY = 4095;

    int targetSpeed = ESC_NEUTRAL_MICROS;
    int diff = rawY - THROTTLE_CENTER_RAW;
    if (diff < 0) diff = -diff;

    if (diff > JOY_DEADZONE_Y) {
        if (rawY < THROTTLE_CENTER_RAW) {
            // Equivalent to: map(rawY, THROTTLE_JOY_MIN, THROTTLE_CENTER_RAW, ESC_MIN_MICROS, ESC_NEUTRAL_MICROS)
            targetSpeed = static_cast<int>(static_cast<long>(rawY - THROTTLE_JOY_MIN) * (ESC_NEUTRAL_MICROS - ESC_MIN_MICROS)
                          / (THROTTLE_CENTER_RAW - THROTTLE_JOY_MIN)) + ESC_MIN_MICROS;
        } else {
            // Equivalent to: map(rawY, THROTTLE_CENTER_RAW, THROTTLE_JOY_MAX, ESC_NEUTRAL_MICROS, ESC_MAX_MICROS)
            targetSpeed = static_cast<int>(static_cast<long>(rawY - THROTTLE_CENTER_RAW) * (ESC_MAX_MICROS - ESC_NEUTRAL_MICROS)
                          / (THROTTLE_JOY_MAX - THROTTLE_CENTER_RAW)) + ESC_NEUTRAL_MICROS;
        }
    }

    if (targetSpeed < ESC_MIN_MICROS) targetSpeed = ESC_MIN_MICROS;
    if (targetSpeed > ESC_MAX_MICROS) targetSpeed = ESC_MAX_MICROS;

#if THROTTLE_INVERT
    // Mirror around neutral: min<->max swap, neutral stays at 1500
    targetSpeed = ESC_MIN_MICROS + ESC_MAX_MICROS - targetSpeed;
#endif

    return targetSpeed;
}
