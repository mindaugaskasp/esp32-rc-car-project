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
    "Throttle Feel",
    "Steering Feel",
};

void CalibrationFlow::begin() {
    _state = State::Menu;
    _cursor = 0;
    _active = -1;
    _wantsExit = false;
    _yWasUp = _yWasDown = false;
    _throttleSwWas = readButton(THROTTLE_SW_PIN);  // require SW release before it re-registers
    _steeringSwWas = readButton(STEERING_SW_PIN);
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
            // Hold on the result so the user can note the values; a fresh throttle
            // press (back) returns to the menu.
            bool throttleSw = readButton(THROTTLE_SW_PIN);
            if (throttleSw && !_throttleSwWas) {
                _throttleSwWas = throttleSw;
                _state = State::Menu;
                _active = -1;
                showMenu();
                return makeNeutralCommand(static_cast<uint32_t>(millis()));
            }
            _throttleSwWas = throttleSw;
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
    bool throttleSw = readButton(THROTTLE_SW_PIN);
    bool steeringSw = readButton(STEERING_SW_PIN);

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

    // Steering press (enter): launch selected calibration
    if (steeringSw && !_steeringSwWas) {
        _active = _cursor;
        _state = State::Running;
        launchActive();
        _yWasUp = _yWasDown = false;
        _throttleSwWas = readButton(THROTTLE_SW_PIN);  // require SW release before it re-registers
        _steeringSwWas = readButton(STEERING_SW_PIN);
        return makeNeutralCommand(static_cast<uint32_t>(millis()));
    }

    // Throttle press (back): exit calibration to the mode selector
    if (throttleSw && !_throttleSwWas) {
        _wantsExit = true;
    }

    _yWasUp = yUp; _yWasDown = yDown; _throttleSwWas = throttleSw; _steeringSwWas = steeringSw;
    return makeNeutralCommand(static_cast<uint32_t>(millis()));
}

VehicleData CalibrationFlow::updateRunning(int rawX, int rawY) {
    // Throttle press (back): cancel the running calibration and return to menu.
    // The wizard's own step-advance is on steering, so watching throttle here can't
    // collide with it.
    bool throttleSw = readButton(THROTTLE_SW_PIN);
    if (throttleSw && !_throttleSwWas) {
        _state = State::Menu;
        _active = -1;
        _throttleSwWas = readButton(THROTTLE_SW_PIN);  // require SW release before it re-registers
        _steeringSwWas = readButton(STEERING_SW_PIN);
        showMenu();
        return makeNeutralCommand(static_cast<uint32_t>(millis()));
    }
    _throttleSwWas = throttleSw;

    VehicleData data = dispatchUpdate(rawX, rawY);

    if (dispatchIsComplete()) {
        _state = State::ResultPause;
        _throttleSwWas = readButton(THROTTLE_SW_PIN); // require a fresh throttle press to leave the result
    }

    return data;
}

void CalibrationFlow::launchActive() {
    switch (_active) {
        case 0: _joystick.begin(); break;
        case 1: _servoAlign.begin(); break;
        case 2: _responseTuning.begin(TuningAxis::Throttle); break;
        case 3: _responseTuning.begin(TuningAxis::Steering); break;
    }
}

VehicleData CalibrationFlow::dispatchUpdate(int rawX, int rawY) {
    switch (_active) {
        case 0: return _joystick.update(rawX, rawY);
        case 1: return _servoAlign.update(rawX, rawY);
        case 2:
        case 3: return _responseTuning.update(rawX, rawY);
    }
    return makeNeutralCommand(static_cast<uint32_t>(millis()));
}

bool CalibrationFlow::dispatchIsComplete() {
    switch (_active) {
        case 0: return _joystick.isComplete();
        case 1: return _servoAlign.isComplete();
        case 2:
        case 3: return _responseTuning.isComplete();
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
