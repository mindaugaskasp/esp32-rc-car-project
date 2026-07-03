#pragma once

#include <stdint.h>
#include "config/ControlConfig.h"

struct VehicleData {
    int servoPosition; // X axis
    int escSpeed; // Y axis
    uint32_t txTimestampMs; // transmitter millis() at send time, echoed back for RTT
};
static_assert(sizeof(VehicleData) == 12, "VehicleData wire size changed");

// The single definition of a "hold still" command: steering centered and throttle
// at its rest point, so the receiver's center-based servo/ESC maps both read
// neutral. Every keep-alive / probe / ping packet must build its safe-neutral frame
// through this helper — hand-writing {JOYSTICK_CENTER_RAW, JOYSTICK_CENTER_RAW, …}
// is the bug that made the motor creep, since JOYSTICK_CENTER_RAW is NOT the ESC
// neutral (that is THROTTLE_CENTER_RAW). The timestamp varies per send, so this is
// a helper rather than a constant.
inline VehicleData makeNeutralCommand(uint32_t txTimestampMs) {
    return VehicleData{JOYSTICK_CENTER_RAW, THROTTLE_CENTER_RAW, txTimestampMs};
}

struct TelemetryData {
    float batteryVoltage;
    int speedRpm;
    uint32_t echoTimestampMs; // echoed from VehicleData.txTimestampMs for latency calc
};
static_assert(sizeof(TelemetryData) == 12, "TelemetryData wire size changed");