#pragma once

#include <stdint.h>
#include "config/ControlConfig.h"
#include "comm/DataTypes.h"

class ThrottleDeadzoneCalibrationScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t { Adjusting, Confirmed };
    State _state = State::Adjusting;
    int _deadzone = JOY_DEADZONE_Y;
    bool _yWasUp = false;
    unsigned long _yUpStart = 0;

    static const unsigned long CONFIRM_MS = 2000;
    static const int MIN_DZ = 0;
    static const int MAX_DZ = 400;

    void showAdjusting(int rawY);
    void showConfirmed();
};
