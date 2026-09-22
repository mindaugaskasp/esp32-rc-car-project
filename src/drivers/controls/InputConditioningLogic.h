#pragma once
#include <stdint.h>

// Pure joystick input-conditioning — no Arduino/hardware deps so it can be unit
// tested natively. The transmitter runs a raw axis reading through here before
// sending, so the command on the wire already has its deadzone removed and an
// expo + rate response curve applied. The receiver then only needs a linear
// command -> PWM endpoint map. All math is integer per-mille.

// Denominator for the per-mille expo/rate parameters: 1000 == full scale.
static constexpr int RESPONSE_FULL_SCALE = 1000;

// Shapes a raw deflection magnitude (0..span) with an expo curve and a rate cap,
// returning a magnitude in the same raw units (0..span).
//
//   normalized d = magnitude / span (in per-mille)
//   expo blend:   (1 - expo) * d + expo * d^3     softer near center, endpoints kept
//   rate cap:     result * rate                    scales the maximum output down
//
// expo and rate are per-mille (0..RESPONSE_FULL_SCALE). expo=0 is linear; a higher
// expo softens response near center while leaving full deflection unchanged.
// rate=RESPONSE_FULL_SCALE passes full output; a lower rate caps the top output.
//
// Fast-path identity when expo=0 and rate=full: returns the magnitude untouched so
// the default (linear, full-range) behavior is byte-exact and free of the extra
// rounding the normalize/denormalize round-trip would introduce.
inline int shapeDeflection(int magnitude, int span, int expoPerMille, int ratePerMille) {
    if (span <= 0) return 0;
    if (magnitude < 0) magnitude = 0;
    if (magnitude > span) magnitude = span;
    if (expoPerMille < 0) expoPerMille = 0;
    if (expoPerMille > RESPONSE_FULL_SCALE) expoPerMille = RESPONSE_FULL_SCALE;
    if (ratePerMille < 0) ratePerMille = 0;
    if (ratePerMille > RESPONSE_FULL_SCALE) ratePerMille = RESPONSE_FULL_SCALE;

    if (expoPerMille == 0 && ratePerMille == RESPONSE_FULL_SCALE) return magnitude;

    long normalized = static_cast<long>(magnitude) * RESPONSE_FULL_SCALE / span;
    long cube = normalized * normalized / RESPONSE_FULL_SCALE;
    cube = cube * normalized / RESPONSE_FULL_SCALE;
    long shaped = (static_cast<long>(RESPONSE_FULL_SCALE - expoPerMille) * normalized
                   + static_cast<long>(expoPerMille) * cube) / RESPONSE_FULL_SCALE;
    shaped = shaped * ratePerMille / RESPONSE_FULL_SCALE;
    return static_cast<int>(shaped * span / RESPONSE_FULL_SCALE);
}

// Conditions a raw axis reading into the command value the transmitter sends:
// clamped to [minRaw, maxRaw], a symmetric deadzone around center forced to center,
// and each side shaped independently by shapeDeflection. Output stays in raw ADC
// units centered on `center`, so the receiver's linear endpoint map reads it directly.
inline int conditionAxis(int raw, int center, int minRaw, int maxRaw,
                         int deadzone, int expoPerMille, int ratePerMille) {
    if (raw < minRaw) raw = minRaw;
    if (raw > maxRaw) raw = maxRaw;

    int deflection = raw - center;
    int magnitude = deflection < 0 ? -deflection : deflection;
    if (magnitude <= deadzone) return center;

    int span = deflection < 0 ? (center - minRaw) : (maxRaw - center);
    int shaped = shapeDeflection(magnitude, span, expoPerMille, ratePerMille);
    return deflection < 0 ? center - shaped : center + shaped;
}
