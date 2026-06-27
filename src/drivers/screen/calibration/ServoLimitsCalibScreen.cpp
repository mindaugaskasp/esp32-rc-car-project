#include "ServoLimitsCalibScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

static const int _CTR  = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;
static const int _HALF = (_CTR - SERVO_MIN_MICROS < SERVO_MAX_MICROS - _CTR)
    ? (_CTR - SERVO_MIN_MICROS) : (SERVO_MAX_MICROS - _CTR);

int ServoLimitsCalibScreen::rawToMicros(int rawX) const {
    return (int)map((long)rawX, 0, 4095, _CTR - _HALF, _CTR + _HALF);
}

void ServoLimitsCalibScreen::begin() {
    _state = State::TestLeft;
    _yWasUp = false;
    showTest(2048, true);
}

VehicleData ServoLimitsCalibScreen::update(int rawX, int rawY) {
    if (_state == State::Complete) { showResult(); return {2048, 2048}; }

    unsigned long now = millis();
    bool yUp = rawY > 3500;
    if (yUp && !_yWasUp) _yUpStart = now;

    if (yUp && (now - _yUpStart >= CONFIRM_MS)) {
        if (_state == State::TestLeft) {
            _leftRaw = rawX;
            _state = State::TestRight;
            _yWasUp = false;
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

bool ServoLimitsCalibScreen::isComplete() const { return _state == State::Complete; }

void ServoLimitsCalibScreen::showTest(int rawX, bool isLeft) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    uint8_t step = isLeft ? 1 : 2;
    const char* inst = isLeft ? "Steer LEFT to stop" : "Steer RIGHT to stop";
    d->displayCalibrationStep("SERVO LIMITS", step, 2, inst, rawX);
}

void ServoLimitsCalibScreen::showResult() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    char l1[28], l2[28];
    snprintf(l1, sizeof(l1), "L limit: ~%d us", rawToMicros(_leftRaw));
    snprintf(l2, sizeof(l2), "R limit: ~%d us", rawToMicros(_rightRaw));
    d->displayCalibrationResult("LIMITS FOUND", l1, l2, "Check Serial for config");
}

void ServoLimitsCalibScreen::printResult() {
    int leftUs = rawToMicros(_leftRaw), rightUs = rawToMicros(_rightRaw);
    debugLogger.logf("[SERVO LIMITS] Left:  %d us (raw %d)", leftUs,  _leftRaw);
    debugLogger.logf("[SERVO LIMITS] Right: %d us (raw %d)", rightUs, _rightRaw);
    debugLogger.logf("[SERVO LIMITS] Set: #define SERVO_MIN_MICROS %d", min(leftUs, rightUs));
    debugLogger.logf("[SERVO LIMITS] Set: #define SERVO_MAX_MICROS %d", max(leftUs, rightUs));
}
