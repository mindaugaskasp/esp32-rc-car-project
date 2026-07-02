#include "ServoCalibrationScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/controls/Controls.h"
#include "config/ControlConfig.h"
#include "config/Esp32Pins.h"
#include <Arduino.h>

static const int CENTER_US = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;
static const int HALF_RANGE =
    (CENTER_US - SERVO_MIN_MICROS < SERVO_MAX_MICROS - CENTER_US)
    ? (CENTER_US - SERVO_MIN_MICROS) : (SERVO_MAX_MICROS - CENTER_US);

int ServoCalibrationScreen::trimDeltaMicros() const {
    return (int)((long)_trimOffsetRaw * 2 * HALF_RANGE / 4095);
}

int ServoCalibrationScreen::rawToMicros(int rawX) const {
    return (int)map((long)rawX, 0, 4095, CENTER_US - HALF_RANGE, CENTER_US + HALF_RANGE);
}

void ServoCalibrationScreen::begin() {
    _state = State::AlignCenter;
    _trimOffsetRaw = 0;
    _leftRaw = 0;
    _rightRaw = 0;
    _yWasUp = _yWasDown = _sw1Was = false;
    showAlignCenter();
}

VehicleData ServoCalibrationScreen::update(int rawX, int rawY) {
    if (_state == State::Complete) { showResult(); return {2048, 2048}; }

    unsigned long now = millis();
    bool yUp = rawY > 3500;
    bool yDown = rawY < 500;
    bool sw1 = readButton(JOY2_SW_PIN); // SW2 (throttle stick) = confirm

    if (yUp && !_yWasUp) _yUpStart = now;
    if (yDown && !_yWasDown) _yDownStart = now;

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
            if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS))
                _trimOffsetRaw = constrain(_trimOffsetRaw + STEP_RAW, -MAX_OFFSET, MAX_OFFSET);
            if (!yDown && _yWasDown && (now - _yDownStart < TAP_MAX_MS))
                _trimOffsetRaw = constrain(_trimOffsetRaw - STEP_RAW, -MAX_OFFSET, MAX_OFFSET);
            // SW1 press: trim done, move on to left limit
            if (sw1 && !_sw1Was) {
                _state = State::TestLeft;
                _yWasUp = _yWasDown = _sw1Was = false;
                _yUpStart = now; // reset hold timer for next phase
                return {2048 + _trimOffsetRaw, 2048};
            }
            showTrimCenter();
            _yWasUp = yUp; _yWasDown = yDown; _sw1Was = sw1;
            return {2048 + _trimOffsetRaw, 2048};
        }

        case State::TestLeft: {
            showTestLimit(true, rawX);
            if (yUp && (now - _yUpStart >= HOLD_MS)) {
                _leftRaw = rawX;
                _state = State::TestRight;
                _yUpStart = now; // reset so user must hold again for right limit
            }
            _yWasUp = yUp; _yWasDown = yDown; _sw1Was = sw1;
            return {rawX, 2048};
        }

        case State::TestRight: {
            showTestLimit(false, rawX);
            if (yUp && (now - _yUpStart >= HOLD_MS)) {
                _rightRaw = rawX;
                _state = State::Complete;
                printResult();
                showResult();
            }
            _yWasUp = yUp; _yWasDown = yDown; _sw1Was = sw1;
            return {rawX, 2048};
        }

        case State::Complete:
            break;
    }

    _yWasUp = yUp; _yWasDown = yDown; _sw1Was = sw1;
    return {2048, 2048};
}

bool ServoCalibrationScreen::isComplete() const { return _state == State::Complete; }

void ServoCalibrationScreen::showAlignCenter() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    drawCalibrationStep(*driver,"SERVO ALIGNMENT", 1, 4,
        "Wheels straight?\nY-up tap: proceed", -1);
}

void ServoCalibrationScreen::showTrimCenter() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char instruction[40];
    snprintf(instruction, sizeof(instruction), "Trim: %+d us\nSW2: done, set lims", trimDeltaMicros());
    drawCalibrationStep(*driver,"SERVO ALIGNMENT", 2, 4, instruction,
        constrain(2048 + _trimOffsetRaw, 0, 4095));
}

void ServoCalibrationScreen::showTestLimit(bool isLeft, int rawX) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    uint8_t step = isLeft ? 3 : 4;
    const char* instruction = isLeft
        ? "Steer LEFT, stop\nY-hold to confirm"
        : "Steer RIGHT, stop\nY-hold to confirm";
    drawCalibrationStep(*driver,"SERVO ALIGNMENT", step, 4, instruction, rawX);
}

void ServoCalibrationScreen::showResult() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    int newTrim = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    int minMicros = min(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    int maxMicros = max(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    char line1[28], line2[28];
    snprintf(line1, sizeof(line1), "Trim: %d us", newTrim);
    snprintf(line2, sizeof(line2), "Min:%d Max:%d", minMicros, maxMicros);
    drawCalibrationResult(*driver,"SERVO CAL DONE", line1, line2, "Edit ControlConfig.h");
}

void ServoCalibrationScreen::printResult() {
    int newTrim = SERVO_CENTER_TRIM_MICROS + trimDeltaMicros();
    int minMicros = min(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    int maxMicros = max(rawToMicros(_leftRaw), rawToMicros(_rightRaw));
    debugLogger.logf("[SERVO] #define SERVO_CENTER_TRIM_MICROS %d", newTrim);
    debugLogger.logf("[SERVO] #define SERVO_MIN_MICROS %d", minMicros);
    debugLogger.logf("[SERVO] #define SERVO_MAX_MICROS %d", maxMicros);
}
