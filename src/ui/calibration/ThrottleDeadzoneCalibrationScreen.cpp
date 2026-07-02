#include "ThrottleDeadzoneCalibrationScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

void ThrottleDeadzoneCalibrationScreen::begin() {
    _state = State::Adjusting;
    _deadzone = JOY_DEADZONE_Y;
    _yWasUp = false;
    showAdjusting(2048);
}

VehicleData ThrottleDeadzoneCalibrationScreen::update(int rawX, int rawY) {
    if (_state == State::Confirmed) { showConfirmed(); return {2048, 2048}; }

    _deadzone = (int)map((long)rawX, 0, 4095, MIN_DZ, MAX_DZ);

    unsigned long now = millis();
    bool yUp = rawY > 3500;
    if (yUp && !_yWasUp) _yUpStart = now;

    if (yUp && (now - _yUpStart >= CONFIRM_MS)) {
        _state = State::Confirmed;
        debugLogger.logf("[THR DEADZONE] Set: #define JOY_DEADZONE_Y %d", _deadzone);
        showConfirmed();
        return {2048, 2048};
    }

    _yWasUp = yUp;
    showAdjusting(rawY);
    return {2048, 2048};
}

bool ThrottleDeadzoneCalibrationScreen::isComplete() const { return _state == State::Confirmed; }

void ThrottleDeadzoneCalibrationScreen::showAdjusting(int rawY) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    bool suppressed = abs(rawY - 2048) <= _deadzone;
    char instruction[32];
    snprintf(instruction, sizeof(instruction), "Zone: +/-%d\nY: %s", _deadzone, suppressed ? "SUPPRESSED" : "LIVE");
    drawCalibrationStep(*driver,"THROTTLE DEADZONE", 1, 1, instruction,
        (int)map((long)_deadzone, MIN_DZ, MAX_DZ, 0, 4095));
}

void ThrottleDeadzoneCalibrationScreen::showConfirmed() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char line1[28];
    snprintf(line1, sizeof(line1), "JOY_DEADZONE_Y = %d", _deadzone);
    drawCalibrationResult(*driver,"THROTTLE DEADZONE", line1, "Edit ControlConfig.h", "then reflash.");
}
