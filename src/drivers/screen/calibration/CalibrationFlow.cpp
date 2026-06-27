#include "CalibrationFlow.h"
#include "drivers/screen/ScreenDriver.h"
#include <Arduino.h>

CalibrationFlow calibrationFlow;

const char* const CalibrationFlow::ITEM_NAMES[ITEM_COUNT] = {
    "ESC Range",
    "ESC Motor Dir",
    "ESC Defaults",
    "ESC Tuning",
    "Joystick Axis",
    "Servo Alignment",
    "Low Voltage",
    "Throttle Deadzone",
};

void CalibrationFlow::begin() {
    _state  = State::Menu;
    _cursor = 0;
    _active = -1;
    _yWasUp = _yWasDown = _xWasRight = _xWasLeft = false;
    showMenu();
}

VehicleData CalibrationFlow::update(int rawX, int rawY) {
    switch (_state) {
        case State::Menu:       return updateMenu(rawX, rawY);
        case State::Running:    return updateRunning(rawX, rawY);
        case State::ResultPause:
            if (millis() - _resultPauseStart >= RESULT_PAUSE_MS) {
                _state = State::Menu;
                showMenu();
            } else {
                dispatchUpdate(2048, 2048);  // keeps result screen refreshed for scrolling
            }
            return {2048, 2048};
    }
    return {2048, 2048};
}

VehicleData CalibrationFlow::updateMenu(int rawX, int rawY) {
    unsigned long now = millis();
    bool yUp    = rawY > 3500;
    bool yDown  = rawY < 500;
    bool xRight = rawX > 3500;

    if (yUp    && !_yWasUp)    _yUpStart    = now;
    if (yDown  && !_yWasDown)  _yDownStart  = now;
    if (xRight && !_xWasRight) _xRightStart = now;

    // Y-tap: navigate cursor
    if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS)) {
        _cursor = (_cursor > 0) ? _cursor - 1 : ITEM_COUNT - 1;
        showMenu();
    }
    if (!yDown && _yWasDown && (now - _yDownStart < TAP_MAX_MS)) {
        _cursor = (_cursor < ITEM_COUNT - 1) ? _cursor + 1 : 0;
        showMenu();
    }

    // X-right tap: launch selected calibration
    if (!xRight && _xWasRight && (now - _xRightStart < TAP_MAX_MS)) {
        _active = _cursor;
        _state  = State::Running;
        launchActive();
        _yWasUp = _yWasDown = _xWasRight = false;
        return {2048, 2048};
    }

    _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
    return {2048, 2048};
}

VehicleData CalibrationFlow::updateRunning(int rawX, int rawY) {
    unsigned long now = millis();
    bool xLeft = rawX < 500;
    if (xLeft && !_xWasLeft) _xLeftStart = now;

    // X-left tap: cancel and return to menu
    if (!xLeft && _xWasLeft && (now - _xLeftStart < TAP_MAX_MS)) {
        _state  = State::Menu;
        _active = -1;
        _xWasLeft = _xWasRight = false;
        showMenu();
        return {2048, 2048};
    }
    _xWasLeft = xLeft;

    VehicleData data = dispatchUpdate(rawX, rawY);

    if (dispatchIsComplete()) {
        _state = State::ResultPause;
        _resultPauseStart = millis();
    }

    return data;
}

void CalibrationFlow::launchActive() {
    switch (_active) {
        case 0: _escRange.begin();    break;
        case 1: _escMotor.begin();    break;
        case 2: _escSetup.begin();    break;
        case 3: _escParam.begin();    break;
        case 4: _joystick.begin();    break;
        case 5: _servoAlign.begin();  break;
        case 6: _lowVoltage.begin();  break;
        case 7: _thrDeadzone.begin(); break;
    }
}

VehicleData CalibrationFlow::dispatchUpdate(int rawX, int rawY) {
    switch (_active) {
        case 0: return _escRange.update(rawX, rawY);
        case 1: return _escMotor.update(rawX, rawY);
        case 2: return _escSetup.update(rawX, rawY);
        case 3: return _escParam.update(rawX, rawY);
        case 4: return _joystick.update(rawX, rawY);
        case 5: return _servoAlign.update(rawX, rawY);
        case 6: return _lowVoltage.update(rawX, rawY);
        case 7: return _thrDeadzone.update(rawX, rawY);
    }
    return {2048, 2048};
}

bool CalibrationFlow::dispatchIsComplete() {
    switch (_active) {
        case 0: return _escRange.isComplete();
        case 1: return _escMotor.isComplete();
        case 2: return _escSetup.isComplete();
        case 3: return _escParam.isComplete();
        case 4: return _joystick.isComplete();
        case 5: return _servoAlign.isComplete();
        case 6: return _lowVoltage.isComplete();
        case 7: return _thrDeadzone.isComplete();
    }
    return false;
}

void CalibrationFlow::showMenu() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    const char* above = (_cursor > 0)              ? ITEM_NAMES[_cursor - 1] : "X>:enter  <X:exit";
    const char* below = (_cursor < ITEM_COUNT - 1) ? ITEM_NAMES[_cursor + 1] : "";

    char selected[26];
    snprintf(selected, sizeof(selected), "> %s", ITEM_NAMES[_cursor]);

    d->displayCalibrationResult("CALIBRATION  Y:nav", above, selected, below);
}
