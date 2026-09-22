#pragma once

// ESC (throttle) output tuning. The ESC PWM endpoints themselves live in
// drivers/esc/EscLogic.h (they are pure computation constants); this file holds the
// output-side tuning knobs. Kept as #define for native-test / Arduino parity.

// Minimum µs change before writing a new speed command to the ESC.
// ESC scale is ~0.2 µs/ADC unit so 5-20 unit noise → 1-4 µs; 4 µs kills it.
#define ESC_DEADBAND_MICROS 4

// Set to true if Y-up reverses the motor instead of driving forward.
// When enabled the ESC output is mirrored around neutral so Y-up = forward.
// NOTE: this only mirrors the ESC output. JOY_INVERT_Y (JoystickConfig.h) flips
// the raw axis — do not enable both for the throttle axis or they cancel out.
#define THROTTLE_INVERT true
