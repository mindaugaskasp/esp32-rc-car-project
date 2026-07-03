#include "CalibrationFlow.h"
#include "drivers/display/ScreenDriver.h"
#include "ui/ScreenUtils.h"
#include "drivers/controls/Controls.h"
#include "config/controller/Esp32Pins.h"
#include "config/ControlConfig.h"
#include <Arduino.h>

CalibrationFlow calibrationFlow;

const char* const CalibrationFlow::ITEM_NAMES[ITEM_COUNT] = {
    "Joystick Axis",
    "Servo Alignment",
    "Throttle Deadzone",
};

void CalibrationFlow::begin() {
    _state = State::Menu;
    _cursor = 0;
    _active = -1;
    _wantsExit = false;
    _yWasUp = _yWasDown = false;
    _sw1Was = readButton(JOY1_SW_PIN);  // require SW release before it re-registers
    _sw2Was = readButton(JOY2_SW_PIN);
    showMenu();
}

bool CalibrationFlow::wantsExit() {
    if (_wantsExit) { _wantsExit = false; return true; }
    return false;
}

VehicleData CalibrationFlow::update(int rawX, int rawY) {
    switch (_state) {
        case State::Menu: return updateMenu(rawX, rawY);
        case State::Running: return updateRunning(rawX, rawY);
        case State::ResultPause: {
            // Hold on the result until SW2 is pressed so the user can note the
            // values; only a fresh SW2 press returns to the menu.
            bool sw2 = readButton(JOY2_SW_PIN);
            if (sw2 && !_sw2Was) {
                _sw2Was = sw2;
                _state = State::Menu;
                _active = -1;
                showMenu();
                return makeNeutralCommand(static_cast<uint32_t>(millis()));
            }
            _sw2Was = sw2;
            dispatchUpdate(ADC_MIDPOINT_RAW, ADC_MIDPOINT_RAW); // keeps result screen refreshed for scrolling
            return makeNeutralCommand(static_cast<uint32_t>(millis()));
        }
    }
    return makeNeutralCommand(static_cast<uint32_t>(millis()));
}

VehicleData CalibrationFlow::updateMenu(int rawX, int rawY) {
    (void)rawX;
    unsigned long now = millis();
    bool yUp = rawY > JOY_GESTURE_UP_RAW;
    bool yDown = rawY < JOY_GESTURE_DOWN_RAW;
    bool sw1 = readButton(JOY1_SW_PIN);
    bool sw2 = readButton(JOY2_SW_PIN);

    if (yUp && !_yWasUp) _yUpStart = now;
    if (yDown && !_yWasDown) _yDownStart = now;

    // Y-tap: navigate cursor
    if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS)) {
        if (_cursor > 0) _cursor--;  // clamp at first item — no wrap
        showMenu();
    }
    if (!yDown && _yWasDown && (now - _yDownStart < TAP_MAX_MS)) {
        if (_cursor < ITEM_COUNT - 1) _cursor++;  // clamp at last item — no wrap
        showMenu();
    }

    // SW1 press (throttle stick): launch selected calibration
    if (sw1 && !_sw1Was) {
        _active = _cursor;
        _state = State::Running;
        launchActive();
        _yWasUp = _yWasDown = false;
        _sw1Was = readButton(JOY1_SW_PIN);  // require SW release before it re-registers
        _sw2Was = readButton(JOY2_SW_PIN);
        return makeNeutralCommand(static_cast<uint32_t>(millis()));
    }

    // SW2 press (steering stick): exit calibration back to mode selector
    if (sw2 && !_sw2Was) {
        _wantsExit = true;
    }

    _yWasUp = yUp; _yWasDown = yDown; _sw1Was = sw1; _sw2Was = sw2;
    return makeNeutralCommand(static_cast<uint32_t>(millis()));
}

VehicleData CalibrationFlow::updateRunning(int rawX, int rawY) {
    bool sw2 = readButton(JOY2_SW_PIN);

    // SW2 press (steering stick): cancel running calibration and return to menu
    if (sw2 && !_sw2Was) {
        _state = State::Menu;
        _active = -1;
        _sw1Was = readButton(JOY1_SW_PIN);  // require SW release before it re-registers
        _sw2Was = readButton(JOY2_SW_PIN);
        showMenu();
        return makeNeutralCommand(static_cast<uint32_t>(millis()));
    }
    _sw2Was = sw2;

    VehicleData data = dispatchUpdate(rawX, rawY);

    if (dispatchIsComplete()) {
        _state = State::ResultPause;
        _sw2Was = readButton(JOY2_SW_PIN); // require a fresh SW2 press to leave the result
    }

    return data;
}

void CalibrationFlow::launchActive() {
    switch (_active) {
        case 0: _joystick.begin(); break;
        case 1: _servoAlign.begin(); break;
        case 2: _throttleDeadzone.begin(); break;
    }
}

VehicleData CalibrationFlow::dispatchUpdate(int rawX, int rawY) {
    switch (_active) {
        case 0: return _joystick.update(rawX, rawY);
        case 1: return _servoAlign.update(rawX, rawY);
        case 2: return _throttleDeadzone.update(rawX, rawY);
    }
    return makeNeutralCommand(static_cast<uint32_t>(millis()));
}

bool CalibrationFlow::dispatchIsComplete() {
    switch (_active) {
        case 0: return _joystick.isComplete();
        case 1: return _servoAlign.isComplete();
        case 2: return _throttleDeadzone.isComplete();
    }
    return false;
}

void CalibrationFlow::showMenu() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    const char* above = (_cursor > 0) ? ITEM_NAMES[_cursor - 1] : nullptr;
    const char* below = (_cursor < ITEM_COUNT - 1) ? ITEM_NAMES[_cursor + 1] : nullptr;

    char selected[26];
    snprintf(selected, sizeof(selected), "> %s", ITEM_NAMES[_cursor]);

    drawCalibrationMenu(*driver, above, selected, below);
}
