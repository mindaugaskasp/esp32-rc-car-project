#include "ServoCalibScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

static const int CENTER_US  = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;
static const int HALF_RANGE =
    (CENTER_US - SERVO_MIN_MICROS < SERVO_MAX_MICROS - CENTER_US)
    ? (CENTER_US - SERVO_MIN_MICROS) : (SERVO_MAX_MICROS - CENTER_US);

int ServoCalibScreen::trimDeltaMicros() const {
    return (int)((long)_trimOffsetRaw * 2 * HALF_RANGE / 4095);
}

int ServoCalibScreen::rawToMicros(int rawX) const {
    return (int)map((long)rawX, 0, 4095, CENTER_US - HALF_RANGE, CENTER_US + HALF_RANGE);
}

void ServoCalibScreen::begin() {
    _state         = State::AlignCenter;
    _trimOffsetRaw = 0;
    _leftRaw       = 0;
    _rightRaw      = 0;
    _yWasUp = _yWasDown = _xWasRight = false;
    showAlignCenter();
}

VehicleData ServoCalibScreen::update(int rawX, int rawY) {
    if (_state == State::Complete) { showResult(); return {2048, 2048}; }

    unsigned long now   = millis();
    bool yUp    = rawY > 3500;
    bool yDown  = rawY < 500;
    bool xRight = rawX > 3500;

    if (yUp    && !_yWasUp)    _yUpStart    = now;
    if (yDown  && !_yWasDown)  _yDownStart  = now;
    if (xRight && !_xWasRight) _xRightStart = now;

    switch (_state) {

        case State::AlignCenter: {
            showAlignCenter();
            // Y-up tap: wheels confirmed straight, proceed to trim
            if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS)) {
                _state = State::TrimCenter;
            }
            break;
        }

        case State::TrimCenter: {
            // Y-tap up/down: nudge center trim
            if (!yUp   && _yWasUp   && (now - _yUpStart   < TAP_MAX_MS))
                _trimOffsetRaw = constrain(_trimOffsetRaw + STEP_RAW, -MAX_OFFSET, MAX_OFFSET);
            if (!yDown && _yWasDown && (now - _yDownStart < TAP_MAX_MS))
                _trimOffsetRaw = constrain(_trimOffsetRaw - STEP_RAW, -MAX_OFFSET, MAX_OFFSET);
            // X-right tap: trim done, move on to left limit
            if (!xRight && _xWasRight && (now - _xRightStart < TAP_MAX_MS)) {
                _state    = State::TestLeft;
                _yWasUp   = _yWasDown = _xWasRight = false;
                _yUpStart = now;  // reset hold timer for next phase
                return {2048 + _trimOffsetRaw, 2048};
            }
            showTrimCenter();
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {2048 + _trimOffsetRaw, 2048};
        }

        case State::TestLeft: {
            showTestLimit(true, rawX);
            if (yUp && (now - _yUpStart >= HOLD_MS)) {
                _leftRaw  = rawX;
                _state    = State::TestRight;
                _yUpStart = now;  // reset so user must hold again for right limit
            }
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {rawX, 2048};
        }

        case State::TestRight: {
            showTestLimit(false, rawX);
            if (yUp && (now - _yUpStart >= HOLD_MS)) {
                _rightRaw = rawX;
                _state    = State::Complete;
                printResult();
                showResult();
            }
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {rawX, 2048};
        }

        case State::Complete:
            break;
    }

    _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
    return {2048, 2048};
}

bool ServoCalibScreen::isComplete() const { return _state == State::Complete; }

void ServoCalibScreen::showAlignCenter() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->displayCalibrationStep("SERVO ALIGNMENT", 1, 4,
        "Wheels straight?\nY-up tap: proceed", -1);
}

void ServoCalibScreen::showTrimCenter() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    char inst[40];
    snprintf(inst, sizeof(inst), "Trim: %+d us\nX> done, set limits", trimDeltaMicros());
    d->displayCalibrationStep("SERVO ALIGNMENT", 2, 4, inst,
        constrain(2048 + _trimOffsetRaw, 0, 4095));
}

void ServoCalibScreen::showTestLimit(bool isLeft, int rawX) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    uint8_t step = isLeft ? 3 : 4;
    const char* inst = isLeft
        ? "Steer LEFT, stop\nY-hold to confirm"
        : "Steer RIGHT, stop\nY-hold to confirm";
    d->displayCalibrationStep("SERVO ALIGNMENT", step, 4, inst, rawX);
}

void ServoCalibScreen::showResult() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    int newTrim   = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    int minMicros = min(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    int maxMicros = max(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    char l1[28], l2[28];
    snprintf(l1, sizeof(l1), "Trim: %d us", newTrim);
    snprintf(l2, sizeof(l2), "Min:%d Max:%d", minMicros, maxMicros);
    d->displayCalibrationResult("SERVO CAL DONE", l1, l2, "Edit ControlConfig.h");
}

void ServoCalibScreen::printResult() {
    int newTrim   = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    int minMicros = min(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    int maxMicros = max(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    debugLogger.logf("[SERVO] #define SERVO_CENTER_TRIM_MICROS %d", newTrim);
    debugLogger.logf("[SERVO] #define SERVO_MIN_MICROS %d", minMicros);
    debugLogger.logf("[SERVO] #define SERVO_MAX_MICROS %d", maxMicros);
}
