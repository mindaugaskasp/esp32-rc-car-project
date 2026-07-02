#include "ServoLimitsCalibrationScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

static const int _CTR = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;
static const int _HALF = (_CTR - SERVO_MIN_MICROS < SERVO_MAX_MICROS - _CTR)
    ? (_CTR - SERVO_MIN_MICROS) : (SERVO_MAX_MICROS - _CTR);

int ServoLimitsCalibrationScreen::rawToMicros(int rawX) const {
    return (int)map((long)rawX, 0, 4095, _CTR - _HALF, _CTR + _HALF);
}

void ServoLimitsCalibrationScreen::begin() {
    _state = State::TestLeft;
    _yWasUp = false;
    showTest(2048, true);
}

VehicleData ServoLimitsCalibrationScreen::update(int rawX, int rawY) {
    if (_state == State::Complete) { showResult(); return {2048, 2048}; }

    unsigned long now = millis();
    bool yUp = rawY > 3500;
    if (yUp && !_yWasUp) _yUpStart = now;

    if (yUp && (now - _yUpStart >= CONFIRM_MS)) {
        if (_state == State::TestLeft) {
            _leftRaw = rawX;
            _state = State::TestRight;
            // Require the stick to return to center and be pushed up again before
            // confirming RIGHT — leave _yWasUp false and return so the unconditional
            // update below does not immediately re-arm the confirm timer.
            _yWasUp = false;
            showTest(rawX, false);
            return {rawX, 2048};
        } else {
            _rightRaw = rawX;
            _state = State::Complete;
            printResult();
            showResult();
            return {2048, 2048};
        }
    }

    _yWasUp = yUp;
    showTest(rawX, _state == State::TestLeft);
    return {rawX, 2048};
}

bool ServoLimitsCalibrationScreen::isComplete() const { return _state == State::Complete; }

void ServoLimitsCalibrationScreen::showTest(int rawX, bool isLeft) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    uint8_t step = isLeft ? 1 : 2;
    const char* instruction = isLeft ? "Steer LEFT to stop" : "Steer RIGHT to stop";
    drawCalibrationStep(*driver,"SERVO LIMITS", step, 2, instruction, rawX);
}

void ServoLimitsCalibrationScreen::showResult() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char line1[28], line2[28];
    snprintf(line1, sizeof(line1), "L limit: ~%d us", rawToMicros(_leftRaw));
    snprintf(line2, sizeof(line2), "R limit: ~%d us", rawToMicros(_rightRaw));
    drawCalibrationResult(*driver,"LIMITS FOUND", line1, line2, "Check Serial for config");
}

void ServoLimitsCalibrationScreen::printResult() {
    int leftUs = rawToMicros(_leftRaw), rightUs = rawToMicros(_rightRaw);
    debugLogger.logf("[SERVO LIMITS] Left:  %d us (raw %d)", leftUs, _leftRaw);
    debugLogger.logf("[SERVO LIMITS] Right: %d us (raw %d)", rightUs, _rightRaw);
    debugLogger.logf("[SERVO LIMITS] Set: #define SERVO_MIN_MICROS %d", min(leftUs, rightUs));
    debugLogger.logf("[SERVO LIMITS] Set: #define SERVO_MAX_MICROS %d", max(leftUs, rightUs));
}
