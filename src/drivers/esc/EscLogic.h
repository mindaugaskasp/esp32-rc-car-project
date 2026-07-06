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

// ESC PWM pulse width (µs) for a conditioned throttle command. The transmitter has
// already removed the deadzone and applied the expo/rate curve, so this is a pure
// linear endpoint map around the measured rest point (THROTTLE_CENTER_RAW): below
// center → reverse/brake, above → forward, command == center → neutral. Integer math
// mirrors Arduino map()/constrain() (no Arduino.h); inputs outside [0, 4095] clamp.
// THROTTLE_INVERT mirrors the output around neutral (Y-up = forward).
inline int computeEscMicros(int command) {
    if (command < 0) command = 0;
    if (command > 4095) command = 4095;

    int targetSpeed = ESC_NEUTRAL_MICROS;
    if (command < THROTTLE_CENTER_RAW) {
        // Equivalent to: map(command, THROTTLE_JOY_MIN, THROTTLE_CENTER_RAW, ESC_MIN_MICROS, ESC_NEUTRAL_MICROS)
        targetSpeed = static_cast<int>(static_cast<long>(command - THROTTLE_JOY_MIN) * (ESC_NEUTRAL_MICROS - ESC_MIN_MICROS)
                      / (THROTTLE_CENTER_RAW - THROTTLE_JOY_MIN)) + ESC_MIN_MICROS;
    } else if (command > THROTTLE_CENTER_RAW) {
        // Equivalent to: map(command, THROTTLE_CENTER_RAW, THROTTLE_JOY_MAX, ESC_NEUTRAL_MICROS, ESC_MAX_MICROS)
        targetSpeed = static_cast<int>(static_cast<long>(command - THROTTLE_CENTER_RAW) * (ESC_MAX_MICROS - ESC_NEUTRAL_MICROS)
                      / (THROTTLE_JOY_MAX - THROTTLE_CENTER_RAW)) + ESC_NEUTRAL_MICROS;
    }

    if (targetSpeed < ESC_MIN_MICROS) targetSpeed = ESC_MIN_MICROS;
    if (targetSpeed > ESC_MAX_MICROS) targetSpeed = ESC_MAX_MICROS;

#if THROTTLE_INVERT
    // Mirror around neutral: min<->max swap, neutral stays at 1500
    targetSpeed = ESC_MIN_MICROS + ESC_MAX_MICROS - targetSpeed;
#endif

    return targetSpeed;
}
