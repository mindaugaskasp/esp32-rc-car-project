#include "ServoTrimCalibrationScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

static const int SERVO_CENTER_US = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;
static const int HALF_RANGE =
    (SERVO_CENTER_US - SERVO_MIN_MICROS < SERVO_MAX_MICROS - SERVO_CENTER_US)
    ? (SERVO_CENTER_US - SERVO_MIN_MICROS)
    : (SERVO_MAX_MICROS - SERVO_CENTER_US);

int ServoTrimCalibrationScreen::trimDeltaMicros() const {
    return (int)((long)_trimOffsetRaw * 2 * HALF_RANGE / 4095);
}

void ServoTrimCalibrationScreen::begin() {
    _state = State::Adjusting;
    _trimOffsetRaw = 0;
    _yWasUp = _yWasDown = false;
    showAdjusting(2048);
}

VehicleData ServoTrimCalibrationScreen::update(int rawX, int rawY) {
    if (_state == State::Confirmed) { showConfirmed(); return {2048, 2048}; }

    unsigned long now = millis();
    bool yUp = rawY > 3500, yDown = rawY < 500;

    if (yUp && !_yWasUp) _yUpStart = now;
    if (yDown && !_yWasDown) _yDownStart = now;

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

bool ServoTrimCalibrationScreen::isComplete() const { return _state == State::Confirmed; }

void ServoTrimCalibrationScreen::showAdjusting(int rawX) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char instruction[28];
    snprintf(instruction, sizeof(instruction), "Trim delta: %+d us", trimDeltaMicros());
    drawCalibrationStep(*driver,"SERVO TRIM CAL", 1, 1, instruction, rawX);
}

void ServoTrimCalibrationScreen::showConfirmed() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    int newTrim = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    char line1[32], line2[32];
    snprintf(line1, sizeof(line1), "Delta: %+d us", trimDeltaMicros());
    snprintf(line2, sizeof(line2), "New trim: %d us", newTrim);
    drawCalibrationResult(*driver,"SERVO TRIM DONE", line1, line2, "Check Serial for config");
}

void ServoTrimCalibrationScreen::printResult() {
    int newTrim = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    debugLogger.logf("[SERVO TRIM] Delta: %+d us", trimDeltaMicros());
    debugLogger.logf("[SERVO TRIM] Set: #define SERVO_CENTER_TRIM_MICROS %d", newTrim);
}
