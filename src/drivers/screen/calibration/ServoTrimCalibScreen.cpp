#include "ServoTrimCalibScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

static const int SERVO_CENTER_US = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;
static const int HALF_RANGE =
    (SERVO_CENTER_US - SERVO_MIN_MICROS < SERVO_MAX_MICROS - SERVO_CENTER_US)
    ? (SERVO_CENTER_US - SERVO_MIN_MICROS)
    : (SERVO_MAX_MICROS - SERVO_CENTER_US);

int ServoTrimCalibScreen::trimDeltaMicros() const {
    return (int)((long)_trimOffsetRaw * 2 * HALF_RANGE / 4095);
}

void ServoTrimCalibScreen::begin() {
    _state = State::Adjusting;
    _trimOffsetRaw = 0;
    _yWasUp = _yWasDown = false;
    showAdjusting(2048);
}

VehicleData ServoTrimCalibScreen::update(int rawX, int rawY) {
    if (_state == State::Confirmed) { showConfirmed(); return {2048, 2048}; }

    unsigned long now = millis();
    bool yUp = rawY > 3500, yDown = rawY < 500;

    if (yUp   && !_yWasUp)   _yUpStart   = now;
    if (yDown && !_yWasDown) _yDownStart  = now;

    if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS))
        _trimOffsetRaw = constrain(_trimOffsetRaw + STEP_RAW, -MAX_OFFSET, MAX_OFFSET);
    if (!yDown && _yWasDown && (now - _yDownStart < TAP_MAX_MS))
        _trimOffsetRaw = constrain(_trimOffsetRaw - STEP_RAW, -MAX_OFFSET, MAX_OFFSET);

    if (yUp && (now - _yUpStart >= CONFIRM_MS)) {
        _state = State::Confirmed;
        printResult();
        showConfirmed();
        return {2048, 2048};
    }

    _yWasUp = yUp; _yWasDown = yDown;
    showAdjusting(rawX);
    return {constrain(rawX + _trimOffsetRaw, 0, 4095), 2048};
}

bool ServoTrimCalibScreen::isComplete() const { return _state == State::Confirmed; }

void ServoTrimCalibScreen::showAdjusting(int rawX) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    char inst[28];
    snprintf(inst, sizeof(inst), "Trim delta: %+d us", trimDeltaMicros());
    d->displayCalibrationStep("SERVO TRIM CAL", 1, 1, inst, rawX);
}

void ServoTrimCalibScreen::showConfirmed() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    int newTrim = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    char l1[32], l2[32];
    snprintf(l1, sizeof(l1), "Delta: %+d us", trimDeltaMicros());
    snprintf(l2, sizeof(l2), "New trim: %d us", newTrim);
    d->displayCalibrationResult("SERVO TRIM DONE", l1, l2, "Check Serial for config");
}

void ServoTrimCalibScreen::printResult() {
    int newTrim = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    debugLogger.logf("[SERVO TRIM] Delta: %+d us", trimDeltaMicros());
    debugLogger.logf("[SERVO TRIM] Set: #define SERVO_CENTER_TRIM_MICROS %d", newTrim);
}
