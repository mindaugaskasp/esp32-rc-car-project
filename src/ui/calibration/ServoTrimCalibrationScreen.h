#pragma once

#include <stdint.h>
#include "comm/DataTypes.h"

class ServoTrimCalibrationScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t { Adjusting, Confirmed };
    State _state = State::Adjusting;
    int _trimOffsetRaw = 0;
    bool _yWasUp = false, _yWasDown = false;
    unsigned long _yUpStart = 0, _yDownStart = 0;

    static const int STEP_RAW = 10;
    static const int MAX_OFFSET = 400;
    static const unsigned long TAP_MAX_MS = 800;
    static const unsigned long CONFIRM_MS = 3000;

    int trimDeltaMicros() const;
    void showAdjusting(int rawX);
    void showConfirmed();
    void printResult();
};
