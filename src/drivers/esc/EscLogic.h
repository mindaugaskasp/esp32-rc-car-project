#pragma once
#include "config/ControlConfig.h"

// ESC PWM pulse width constants (microseconds)
static const int ESC_MIN_MICROS     = 1100;
static const int ESC_NEUTRAL_MICROS = 1500;
static const int ESC_MAX_MICROS     = 1900;

// Raw joystick-Y band that forces ESC neutral (throttle deadzone). Kept as the
// original asymmetric window around center (~2048): slightly wider on the forward
// side. Named here rather than inlined so the neutral band is not a magic number.
static const int ESC_NEUTRAL_JOY_LOW  = 1900;
static const int ESC_NEUTRAL_JOY_HIGH = 2200;

// Computes the ESC PWM pulse width (µs) for a raw 12-bit joystick Y input.
// Matches Arduino map() / constrain() integer arithmetic exactly — no Arduino.h needed.
// Inputs outside [0, 4095] are clamped. Values in (1900, 2200) map to neutral.
// When THROTTLE_INVERT is true the output is mirrored around neutral (Y-up = forward).
inline int computeEscMicros(int rawY) {
    if (rawY < 0)    rawY = 0;
    if (rawY > 4095) rawY = 4095;

    // Equivalent to: map(rawY, 0, 4095, ESC_MIN_MICROS, ESC_MAX_MICROS)
    int targetSpeed = (int)((long)rawY * (ESC_MAX_MICROS - ESC_MIN_MICROS) / 4095) + ESC_MIN_MICROS;

    // Deadzone around joystick center -> force neutral
    if (rawY > ESC_NEUTRAL_JOY_LOW && rawY < ESC_NEUTRAL_JOY_HIGH) {
        targetSpeed = ESC_NEUTRAL_MICROS;
    }

#if THROTTLE_INVERT
    // Mirror around neutral: min<->max swap, neutral stays at 1500
    targetSpeed = ESC_MIN_MICROS + ESC_MAX_MICROS - targetSpeed;
#endif

    return targetSpeed;
}
