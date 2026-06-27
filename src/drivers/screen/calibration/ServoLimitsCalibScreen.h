#pragma once

#include <stdint.h>
#include "drivers/wifi/DataTypes.h"

class ServoLimitsCalibScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t { TestLeft, TestRight, Complete };
    State _state = State::TestLeft;
    int  _leftRaw = 0, _rightRaw = 0;
    bool _yWasUp = false;
    unsigned long _yUpStart = 0;
    static const unsigned long CONFIRM_MS = 1000;

    int  rawToMicros(int rawX) const;
    void showTest(int rawX, bool isLeft);
    void showResult();
    void printResult();
};
