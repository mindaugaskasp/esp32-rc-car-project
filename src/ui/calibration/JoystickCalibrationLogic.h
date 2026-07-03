#pragma once
#include <stdint.h>

// Pure joystick-calibration inference — no Arduino/hardware deps so it can be unit
// tested natively. The screen samples the stick and feeds the raw numbers here to
// derive the values a user should copy into ControlConfig.h.

// Deadzone suggestion tuning. A per-axis deadzone must comfortably exceed the
// joystick's measured resting jitter so a released stick never drifts past it.
static const int CALIBRATION_DEADZONE_SAFETY_MARGIN = 2; // × the measured rest deviation
static const int CALIBRATION_DEADZONE_MIN = 20;          // floor for a usable deadzone
// Real at-rest jitter is only tens of ADC counts. Cap the deviation the suggestion
// is built from so a single stray reading (a glitch, or the stick bumped during
// centering) cannot blow the deadzone past a usable value.
static const int CALIBRATION_MAX_REST_DEVIATION = 150;

// Mid-point of a 12-bit ADC, used as the safe fallback center when no samples were
// taken (a missing measurement should read as neutral, not 0).
static const int CALIBRATION_ADC_MIDPOINT = 2048;
// A released, self-centering stick rests near mid-scale. Any centering-phase sample
// further than this from the mid-point is a deflection or glitch, not rest, and is
// excluded from the jitter measurement so it cannot skew the deadzone suggestion.
static const int CALIBRATION_REST_MAX_OFFSET = 900;

// Integer mean of the resting samples → the axis center raw value. This is what a
// user copies into JOYSTICK_CENTER_RAW (X) / THROTTLE_CENTER_RAW (Y).
inline int computeCenterRaw(long sampleSum, int sampleCount) {
    if (sampleCount <= 0) return CALIBRATION_ADC_MIDPOINT;
    return static_cast<int>(sampleSum / sampleCount);
}

// True when a centering-phase sample is close enough to mid-scale to be genuine
// rest jitter rather than a bump/glitch. The screen gates its jitter min/max on
// this so one stray full-deflection reading cannot corrupt the deadzone suggestion.
inline bool isPlausibleRestSample(int raw) {
    int offset = raw - CALIBRATION_ADC_MIDPOINT;
    if (offset < 0) offset = -offset;
    return offset <= CALIBRATION_REST_MAX_OFFSET;
}

// Suggested deadzone for one axis, inferred from its resting jitter: the largest
// deviation of any at-rest sample from the center, clamped to a plausible ceiling,
// scaled by a safety margin, and floored so the suggestion is always usable.
// restMin/restMax are the extremes seen while the stick was released.
inline int suggestDeadzone(int center, int restMin, int restMax) {
    int lowDeviation = center - restMin;
    int highDeviation = restMax - center;
    int deviation = lowDeviation > highDeviation ? lowDeviation : highDeviation;
    if (deviation < 0) deviation = 0;
    if (deviation > CALIBRATION_MAX_REST_DEVIATION) deviation = CALIBRATION_MAX_REST_DEVIATION;
    int suggested = deviation * CALIBRATION_DEADZONE_SAFETY_MARGIN;
    if (suggested < CALIBRATION_DEADZONE_MIN) suggested = CALIBRATION_DEADZONE_MIN;
    return suggested;
}
