#pragma once
#include "config/ControlConfig.h"

// Pure battery-voltage computation — no Arduino dep, testable natively.
// The driver feeds it calibrated ADC millivolts (analogReadMilliVolts);
// everything here is plain integer arithmetic in millivolts.

// Scales the divider tap voltage back up to battery voltage:
// Vbat = Vpin × (TOP + BOTTOM) / BOTTOM. Guards nonsense divider values.
inline int batteryMillivoltsFromAdc(int adcMillivolts, long dividerTopOhms, long dividerBottomOhms) {
    if (adcMillivolts < 0) adcMillivolts = 0;
    if (dividerTopOhms < 0 || dividerBottomOhms <= 0) return 0;
    return static_cast<int>(static_cast<long>(adcMillivolts) * (dividerTopOhms + dividerBottomOhms)
           / dividerBottomOhms);
}

// Per-board trim for resistor tolerance / residual ADC error (1000 = identity).
inline int applyBatteryCalibration(int millivolts, int calibrationPerMille) {
    if (millivolts < 0 || calibrationPerMille <= 0) return 0;
    return static_cast<int>(static_cast<long>(millivolts) * calibrationPerMille / 1000);
}

// Low-voltage warning latch with hysteresis: trips below the threshold, clears
// only after recovering above threshold + hysteresis. Nonpositive readings mean
// "no measurement yet" (e.g. telemetry not received) and never warn.
inline bool updateLowVoltageWarning(bool wasLow, int millivolts,
                                    int lowThresholdMillivolts, int hysteresisMillivolts) {
    if (millivolts <= 0) return false;
    if (wasLow) return millivolts < lowThresholdMillivolts + hysteresisMillivolts;
    return millivolts < lowThresholdMillivolts;
}

// EMA across samples; NUM/DEN = weight of the newest sample. A negative
// previous value means "filter not primed yet" and adopts the sample directly.
// A nonzero delta always steps at least 1mV — integer rounding would otherwise
// park the filter 1mV away from a steady input forever.
inline int smoothBatteryMillivolts(int previousMillivolts, int sampleMillivolts,
                                   int smoothingNum, int smoothingDen) {
    if (previousMillivolts < 0) return sampleMillivolts;
    if (smoothingDen <= 0 || smoothingNum <= 0 || smoothingNum >= smoothingDen) return sampleMillivolts;
    long weightedDelta = static_cast<long>(sampleMillivolts - previousMillivolts) * smoothingNum;
    long rounded = weightedDelta >= 0 ? weightedDelta + smoothingDen / 2 : weightedDelta - smoothingDen / 2;
    long step = rounded / smoothingDen;
    if (step == 0 && sampleMillivolts != previousMillivolts) {
        step = sampleMillivolts > previousMillivolts ? 1 : -1;
    }
    return previousMillivolts + static_cast<int>(step);
}
