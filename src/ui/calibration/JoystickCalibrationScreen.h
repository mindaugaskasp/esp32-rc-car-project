#pragma once

#include <stdint.h>
#include "comm/DataTypes.h"

class JoystickCalibrationScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    // Per-axis flow: measure center, then sweep X, then sweep Y as separate steps
    // so each axis is calibrated on its own and its low/high is shown in isolation.
    // The two sweep steps are advanced manually with SW1 (no timer) so the user can
    // take their time hitting each extreme.
    enum class State : uint8_t { Release, MeasureCenter, SweepX, SweepY, Complete };
    State _state = State::Release;
    unsigned long _stateEnteredAt = 0;
    long _sumX = 0, _sumY = 0;
    int _sampleCount = 0;
    int _centerX = ADC_MIDPOINT_RAW, _centerY = ADC_MIDPOINT_RAW;
    int _minX = ADC_MAX_RAW, _maxX = 0;
    int _minY = ADC_MAX_RAW, _maxY = 0;
    // Resting jitter captured during MeasureCenter — drives the suggested deadzone.
    int _restMinX = ADC_MAX_RAW, _restMaxX = 0;
    int _restMinY = ADC_MAX_RAW, _restMaxY = 0;
    bool _sw1Was = false; // SW1 (throttle stick) advances a sweep step

    void showRelease(int secondsLeft);
    void showMeasuring(int progressRaw);
    void showSweepX(int rawX);
    void showSweepY(int rawY);
    void showResult();
    void printResults();
};
