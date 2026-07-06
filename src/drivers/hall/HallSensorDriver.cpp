#include "HallSensorDriver.h"
#include "HallLogic.h"
#include "config/controller/Esp32Pins.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

constexpr unsigned long HALL_RPM_INTERVAL_US = HALL_RPM_INTERVAL_MS * 1000UL;

// Pulse counter incremented from ISR — must be volatile.
static volatile uint32_t _pulseCount = 0;

// micros() timestamp of the last accepted pulse: ISR debounce reference and, for
// the driver, the "how long since a pulse" stop check.
static volatile unsigned long _lastPulseUs = 0;

// micros() between the two most recent accepted pulses — the low-speed timing
// source. Zero until at least two pulses have been seen.
static volatile uint32_t _lastPulseIntervalUs = 0;

// Reported RPM after smoothing, refreshed each window (or forced to 0 on stop).
static int _currentRpm = 0;

static unsigned long _windowStartUs = 0;

void IRAM_ATTR hallSensorISR() {
    unsigned long now = micros();
    unsigned long sinceLastPulseUs = now - _lastPulseUs;
    if (sinceLastPulseUs >= HALL_MIN_PULSE_INTERVAL_US) {
        // Skip the very first pulse (and the first after boot): _lastPulseUs == 0
        // makes sinceLastPulseUs a meaningless huge value, not a real interval.
        if (_lastPulseUs != 0) {
            _lastPulseIntervalUs = static_cast<uint32_t>(sinceLastPulseUs);
        }
        _pulseCount++;
        _lastPulseUs = now;
    }
}

void initHallSensor() {
    // Active-low sensor (3144: HIGH=no magnet, LOW=detected). Enable the internal
    // pull-up so the line idles HIGH: a disconnected or idle sensor then emits no
    // spurious FALLING edges. A bare INPUT floats, and EMI noise on the floating pin
    // triggers the ISR — the source of phantom RPM when no sensor is attached.
    pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hallSensorISR, FALLING);
    _windowStartUs = micros();
}

void updateHallSensor() {
    unsigned long now = micros();

    noInterrupts();
    unsigned long lastPulseUs = _lastPulseUs;
    interrupts();

    // Definitively stopped: force the reported RPM to 0 now rather than waiting for
    // the next window or letting the EMA coast down from the last speed. This also
    // clears the filter state (_currentRpm is the only state), so the next launch
    // ramps up from rest instead of from a stale reading.
    if (lastPulseUs == 0 || now - lastPulseUs > HALL_STOP_TIMEOUT_US) {
        _currentRpm = 0;
        _windowStartUs = now;
        noInterrupts();
        _pulseCount = 0;
        interrupts();
        return;
    }

    unsigned long windowElapsedUs = now - _windowStartUs;
    if (windowElapsedUs < HALL_RPM_INTERVAL_US) return;

    noInterrupts();
    uint32_t count = _pulseCount;
    uint32_t intervalUs = _lastPulseIntervalUs;
    _pulseCount = 0;
    interrupts();

    _windowStartUs = now;

    int sampleRpm = selectWindowRpm(count, windowElapsedUs / 1000, intervalUs, HALL_STOP_TIMEOUT_US);
    _currentRpm = smoothRpm(_currentRpm, sampleRpm);
}

int getMotorRpm() {
    return _currentRpm;
}
