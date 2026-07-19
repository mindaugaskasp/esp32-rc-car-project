#pragma once

#include <stdint.h>
#include "config/ControlConfig.h"
#include "comm/LinkModeLogic.h"

struct VehicleData {
    // Both axes carry a transmitter-conditioned command in raw ADC units (deadzone
    // removed, expo/rate curve applied — see InputConditioningLogic.h), still centered
    // on STEERING_CENTER_RAW / THROTTLE_CENTER_RAW. The receiver applies only a linear
    // command -> PWM endpoint map (ServoLogic.h / EscLogic.h).
    int servoPosition; // X axis (steering command)
    int escSpeed; // Y axis (throttle command)
    uint32_t txTimestampMs; // transmitter millis() at send time, echoed back for RTT
    uint8_t linkMode; // transmitter's DESIRED PHY (LinkPhyMode); stamped on every frame by sendData()
};
static_assert(sizeof(VehicleData) == 16, "VehicleData wire size changed");

// The one definition of a "hold still" command: steering and throttle each at their
// measured rest point so the receiver's center-based maps read neutral. Every
// keep-alive / probe / ping must build its safe-neutral frame here — hand-writing
// {STEERING_CENTER_RAW, STEERING_CENTER_RAW, …} is what made the motor creep, since
// the ESC neutral is THROTTLE_CENTER_RAW, not STEERING_CENTER_RAW.
inline VehicleData makeNeutralCommand(uint32_t txTimestampMs) {
    // linkMode 0 (Standard) here is a placeholder — sendData() overwrites it with
    // the transmitter's current desired mode before the frame goes out.
    return VehicleData{STEERING_CENTER_RAW, THROTTLE_CENTER_RAW, txTimestampMs, 0};
}

struct TelemetryData {
    float batteryVoltage;
    int speedRpm;
    uint32_t echoTimestampMs; // echoed from VehicleData.txTimestampMs for latency calc
    uint8_t linkMode; // receiver's APPLIED PHY (LinkPhyMode); echoed so the transmitter learns of a switch
};
static_assert(sizeof(TelemetryData) == 16, "TelemetryData wire size changed");