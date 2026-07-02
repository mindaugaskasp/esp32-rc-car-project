#pragma once

#include <stdint.h>
#include "comm/DataTypes.h"

class JoystickCalibScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t { Release, MeasureCenter, SweepExtremes, Complete };
    State _state = State::Release;
    unsigned long _stateEnteredAt = 0;
    long _sumX = 0, _sumY = 0;
    int  _sampleCount = 0;
    int  _centerX = 2048, _centerY = 2048;
    int  _minX = 4095, _maxX = 0;
    int  _minY = 4095, _maxY = 0;

    void showRelease(int secondsLeft);
    void showMeasuring(int progressRaw);
    void showSweep(int rawX, int rawY);
    void showResult();
    void printResults();
};
