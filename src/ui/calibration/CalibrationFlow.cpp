#include "CalibrationFlow.h"
#include "drivers/display/ScreenDriver.h"
#include "ui/ScreenUtils.h"
#include "drivers/controls/Controls.h"
#include "config/Esp32Pins.h"
#include <Arduino.h>

CalibrationFlow calibrationFlow;

const char* const CalibrationFlow::ITEM_NAMES[ITEM_COUNT] = {
    "Joystick Axis",
    "Servo Alignment",
    "Throttle Deadzone",
};

void CalibrationFlow::begin() {
    _state      = State::Menu;
    _cursor     = 0;
    _active     = -1;
    _wantsExit  = false;
    _yWasUp = _yWasDown = _sw1Was = _sw2Was = false;
    showMenu();
}

bool CalibrationFlow::wantsExit() {
    if (_wantsExit) { _wantsExit = false; return true; }
    return false;
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
    (void)rawX;
    unsigned long now = millis();
    bool yUp  = rawY > 3500;
    bool yDown = rawY < 500;
    bool sw1  = readButton(JOY1_SW_PIN);
    bool sw2  = readButton(JOY2_SW_PIN);

    if (yUp   && !_yWasUp)   _yUpStart   = now;
    if (yDown && !_yWasDown) _yDownStart = now;

    // Y-tap: navigate cursor
    if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS)) {
        _cursor = (_cursor > 0) ? _cursor - 1 : ITEM_COUNT - 1;
        showMenu();
    }
    if (!yDown && _yWasDown && (now - _yDownStart < TAP_MAX_MS)) {
        _cursor = (_cursor < ITEM_COUNT - 1) ? _cursor + 1 : 0;
        showMenu();
    }

    // SW2 press (throttle stick): launch selected calibration
    if (sw2 && !_sw2Was) {
        _active = _cursor;
        _state  = State::Running;
        launchActive();
        _yWasUp = _yWasDown = _sw1Was = _sw2Was = false;
        return {2048, 2048};
    }

    // SW1 press (steering stick): exit calibration back to mode selector
    if (sw1 && !_sw1Was) {
        _wantsExit = true;
    }

    _yWasUp = yUp; _yWasDown = yDown; _sw1Was = sw1; _sw2Was = sw2;
    return {2048, 2048};
}

VehicleData CalibrationFlow::updateRunning(int rawX, int rawY) {
    bool sw1 = readButton(JOY1_SW_PIN);

    // SW1 press (steering stick): cancel running calibration and return to menu
    if (sw1 && !_sw1Was) {
        _state  = State::Menu;
        _active = -1;
        _sw1Was = _sw2Was = false;
        showMenu();
        return {2048, 2048};
    }
    _sw1Was = sw1;

    VehicleData data = dispatchUpdate(rawX, rawY);

    if (dispatchIsComplete()) {
        _state = State::ResultPause;
        _resultPauseStart = millis();
    }

    return data;
}

void CalibrationFlow::launchActive() {
    switch (_active) {
        case 0: _joystick.begin();    break;
        case 1: _servoAlign.begin();  break;
        case 2: _thrDeadzone.begin(); break;
    }
}

VehicleData CalibrationFlow::dispatchUpdate(int rawX, int rawY) {
    switch (_active) {
        case 0: return _joystick.update(rawX, rawY);
        case 1: return _servoAlign.update(rawX, rawY);
        case 2: return _thrDeadzone.update(rawX, rawY);
    }
    return {2048, 2048};
}

bool CalibrationFlow::dispatchIsComplete() {
    switch (_active) {
        case 0: return _joystick.isComplete();
        case 1: return _servoAlign.isComplete();
        case 2: return _thrDeadzone.isComplete();
    }
    return false;
}

void CalibrationFlow::showMenu() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;

    const char* above = (_cursor > 0)              ? ITEM_NAMES[_cursor - 1] : nullptr;
    const char* below = (_cursor < ITEM_COUNT - 1) ? ITEM_NAMES[_cursor + 1] : nullptr;

    char selected[26];
    snprintf(selected, sizeof(selected), "> %s", ITEM_NAMES[_cursor]);

    drawCalibMenu(*d, above, selected, below);
}
