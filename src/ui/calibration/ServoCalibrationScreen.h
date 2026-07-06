#pragma once

#include <stdint.h>
#include "comm/DataTypes.h"

// Unified servo calibration: physical centering guide → center trim → left limit → right limit.
// Phase 1: Confirm wheels are straight (mechanical setup)
// Phase 2: Y-tap to adjust center trim; X-right tap to proceed
// Phase 3: Steer LEFT to mechanical stop, Y-hold to confirm
// Phase 4: Steer RIGHT to mechanical stop, Y-hold to confirm
class ServoCalibrationScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t { AlignCenter, TrimCenter, TestLeft, TestRight, Complete };
    State _state = State::AlignCenter;

    int _trimOffsetRaw = 0;
    int _leftRaw = 0;
    int _rightRaw = 0;

    bool _yWasUp = false;
    bool _yWasDown = false;
    bool _steeringSwWas = false; // STEERING_SW — confirm trim done, proceed
    unsigned long _yUpStart = 0;
    unsigned long _yDownStart = 0;

    static const int STEP_RAW = 10;
    static const int MAX_OFFSET = 400;
    static const unsigned long TAP_MAX_MS = 700;
    static const unsigned long HOLD_MS = 1000;

    int trimDeltaMicros() const;
    int rawToMicros(int rawX) const;

    void showAlignCenter();
    void showTrimCenter();
    void showTestLimit(bool isLeft, int rawX);
    void showResult();
    void printResult();
};
