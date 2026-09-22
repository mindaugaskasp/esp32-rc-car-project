#pragma once
#include "config/ControlConfig.h"

static constexpr int SERVO_STEERING_CENTER_RAW = STEERING_CENTER_RAW;

// Servo PWM pulse width (µs) for a conditioned steering command. The transmitter has
// already removed the deadzone and applied the expo/rate curve, so this is a pure
// linear endpoint map with center trim and clamping; command == center → trimmed
// neutral. Integer math mirrors Arduino map()/constrain() (no Arduino.h).
inline int computeServoMicros(int command) {
    if (command < 0) command = 0;
    if (command > 4095) command = 4095;

    const int center = SERVO_CENTER_MICROS;

    int targetMicros = center;
    if (command < SERVO_STEERING_CENTER_RAW) {
        // Equivalent to: map(command, STEERING_JOY_MIN, SERVO_STEERING_CENTER_RAW, SERVO_MIN_MICROS, center)
        targetMicros = static_cast<int>(static_cast<long>(command - STEERING_JOY_MIN) * (center - SERVO_MIN_MICROS)
                       / (SERVO_STEERING_CENTER_RAW - STEERING_JOY_MIN)) + SERVO_MIN_MICROS;
    } else if (command > SERVO_STEERING_CENTER_RAW) {
        // Equivalent to: map(command, SERVO_STEERING_CENTER_RAW, STEERING_JOY_MAX, center, SERVO_MAX_MICROS)
        targetMicros = static_cast<int>(static_cast<long>(command - SERVO_STEERING_CENTER_RAW) * (SERVO_MAX_MICROS - center)
                       / (STEERING_JOY_MAX - SERVO_STEERING_CENTER_RAW)) + center;
    }

    if (targetMicros < SERVO_MIN_MICROS) targetMicros = SERVO_MIN_MICROS;
    if (targetMicros > SERVO_MAX_MICROS) targetMicros = SERVO_MAX_MICROS;

    return targetMicros;
}
