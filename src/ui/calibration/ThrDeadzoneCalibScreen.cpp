#include "ThrDeadzoneCalibScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

void ThrDeadzoneCalibScreen::begin() {
    _state = State::Adjusting;
    _deadzone = JOY_DEADZONE_Y;
    _yWasUp = false;
    showAdjusting(2048);
}

VehicleData ThrDeadzoneCalibScreen::update(int rawX, int rawY) {
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

bool ThrDeadzoneCalibScreen::isComplete() const { return _state == State::Confirmed; }

void ThrDeadzoneCalibScreen::showAdjusting(int rawY) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    bool suppressed = abs(rawY - 2048) <= _deadzone;
    char inst[32];
    snprintf(inst, sizeof(inst), "Zone: +/-%d\nY: %s", _deadzone, suppressed ? "SUPPRESSED" : "LIVE");
    drawCalibStep(*d,"THROTTLE DEADZONE", 1, 1, inst,
        (int)map((long)_deadzone, MIN_DZ, MAX_DZ, 0, 4095));
}

void ThrDeadzoneCalibScreen::showConfirmed() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    char l1[28];
    snprintf(l1, sizeof(l1), "JOY_DEADZONE_Y = %d", _deadzone);
    drawCalibResult(*d,"THROTTLE DEADZONE", l1, "Edit ControlConfig.h", "then reflash.");
}
