#include "HallSensorDriver.h"
#include "HallLogic.h"
#include "config/Esp32Pins.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

// Pulse counter incremented from ISR — must be volatile.
static volatile uint32_t _pulseCount = 0;

// Timestamp of the last accepted pulse for ISR-level debounce.
static volatile unsigned long _lastPulseUs = 0;

// Last computed RPM value, updated every HALL_RPM_INTERVAL_MS.
static int _currentRpm = 0;

static unsigned long _lastCalcTime = 0;

void IRAM_ATTR hallSensorISR() {
    unsigned long now = micros();
    if (now - _lastPulseUs >= HALL_MIN_PULSE_INTERVAL_US) {
        _pulseCount++;
        _lastPulseUs = now;
    }
}

void initHallSensor() {
    // Module has built-in pull-up; 3144 output is active-low (HIGH=no magnet, LOW=detected).
    pinMode(HALL_SENSOR_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hallSensorISR, FALLING);
    _lastCalcTime = millis();
}

void updateHallSensor() {
    unsigned long now     = millis();
    unsigned long elapsed = now - _lastCalcTime;

    if (elapsed < HALL_RPM_INTERVAL_MS) return;

    // Atomically snapshot and reset the pulse counter.
    noInterrupts();
    uint32_t count = _pulseCount;
    _pulseCount    = 0;
    interrupts();

    _lastCalcTime = now;

    _currentRpm = computeMotorRpm(count, elapsed);
}

int getMotorRpm() {
    return _currentRpm;
}
