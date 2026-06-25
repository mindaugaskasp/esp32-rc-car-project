#pragma once

struct VehicleData {
    int servoPos; // X axis
    int escSpeed; // Y axis
};


struct TelemetryData {
    float batteryVoltage;
    int speedRpm;
};