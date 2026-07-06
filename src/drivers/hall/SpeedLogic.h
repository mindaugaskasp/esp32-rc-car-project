#pragma once
#include "config/ControlConfig.h"
#include <stdint.h>

// Converts motor-shaft RPM (from the hall sensor) into ground speed in km/h using
// the drivetrain geometry in VehicleConfig.h. Pure float arithmetic, no Arduino
// dependency, so it is unit-testable natively (see test/test_speed_logic).
//
// This is a header (not build-filtered), so the transmitter dashboard can include
// it even though the hall driver itself compiles receiver-only.
//
//   wheelRpm       = motorRpm / gearRatio                 (motor turns per wheel turn)
//   circumference  = PI * wheelDiameterMm / 1000          (metres per wheel turn)
//   km/h           = wheelRpm * circumference * 60 / 1000
//
// Guards a non-positive gear ratio or wheel diameter (divide-by-zero / garbage) by
// reporting 0 — unreachable with today's constants, but a future edit could hit it.
inline float motorRpmToKmh(int motorRpm,
                           float wheelDiameterMm = WHEEL_DIAMETER_MM,
                           float gearRatio = GEAR_RATIO) {
    if (gearRatio <= 0.0f || wheelDiameterMm <= 0.0f) return 0.0f;
    const float piApprox = 3.14159265f;
    const float wheelCircumferenceMeters = piApprox * wheelDiameterMm / 1000.0f;
    const float wheelRpm = motorRpm / gearRatio;
    return wheelRpm * wheelCircumferenceMeters * 60.0f / 1000.0f;
}
