#pragma once

#include <stdint.h>

struct VehicleData {
    int servoPos;          // X axis
    int escSpeed;          // Y axis
    uint32_t txTimestampMs; // transmitter millis() at send time, echoed back for RTT
};
static_assert(sizeof(VehicleData) == 12, "VehicleData wire size changed");

struct TelemetryData {
    float batteryVoltage;
    int speedRpm;
    uint32_t echoTimestampMs; // echoed from VehicleData.txTimestampMs for latency calc
};
static_assert(sizeof(TelemetryData) == 12, "TelemetryData wire size changed");