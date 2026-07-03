#include "ThrottleDeadzoneCalibrationScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

void ThrottleDeadzoneCalibrationScreen::begin() {
    _state = State::Adjusting;
    _deadzone = JOY_DEADZONE_Y;
    _yWasUp = false;
    showAdjusting(ADC_MIDPOINT_RAW);
}

VehicleData ThrottleDeadzoneCalibrationScreen::update(int rawX, int rawY) {
    if (_state == State::Confirmed) {
        showConfirmed();
        return makeNeutralCommand(static_cast<uint32_t>(millis()));
    }

    _deadzone = static_cast<int>(map(static_cast<long>(rawX), 0, ADC_MAX_RAW, MIN_DZ, MAX_DZ));

    unsigned long now = millis();
    bool yUp = rawY > JOY_GESTURE_UP_RAW;
    if (yUp && !_yWasUp) _yUpStart = now;

    if (yUp && (now - _yUpStart >= CONFIRM_MS)) {
        _state = State::Confirmed;
        debugLogger.logf("[THR DEADZONE] Set: #define JOY_DEADZONE_Y %d", _deadzone);
        showConfirmed();
        return makeNeutralCommand(static_cast<uint32_t>(now));
    }

    _yWasUp = yUp;
    showAdjusting(rawY);
    return makeNeutralCommand(static_cast<uint32_t>(now));
}

bool ThrottleDeadzoneCalibrationScreen::isComplete() const { return _state == State::Confirmed; }

void ThrottleDeadzoneCalibrationScreen::showAdjusting(int rawY) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    bool suppressed = abs(rawY - ADC_MIDPOINT_RAW) <= _deadzone;
    char instruction[32];
    snprintf(instruction, sizeof(instruction), "Zone: +/-%d\nY: %s", _deadzone, suppressed ? "SUPPRESSED" : "LIVE");
    drawCalibrationStep(*driver, "THROTTLE DEADZONE", 1, 1, instruction,
        static_cast<int>(map(static_cast<long>(_deadzone), MIN_DZ, MAX_DZ, 0, ADC_MAX_RAW)));
}

void ThrottleDeadzoneCalibrationScreen::showConfirmed() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char line1[28];
    snprintf(line1, sizeof(line1), "JOY_DEADZONE_Y = %d", _deadzone);
    drawCalibrationResult(*driver, "THROTTLE DEADZONE", line1, "Edit ControlConfig.h", "SW2:done, then reflash");
}
