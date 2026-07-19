#include "ResponseTuningScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/controls/Controls.h"
#include "drivers/controls/InputConditioningLogic.h"
#include "drivers/debug/DebugLogger.h"
#include "config/ControlConfig.h"
#include "config/controller/Esp32Pins.h"
#include <Arduino.h>

void ResponseTuningScreen::begin(TuningAxis axis) {
    _axis = axis;
    _state = State::Adjusting;
    _selected = Parameter::Deadzone;
    if (_axis == TuningAxis::Throttle) {
        _deadzone = JOY_DEADZONE_Y;
        _expo = THROTTLE_EXPO;
        _rate = THROTTLE_RATE;
    } else {
        _deadzone = JOY_DEADZONE_X;
        _expo = STEERING_EXPO;
        _rate = STEERING_RATE;
    }
    _pressActive = false;
    _steeringSwDownAt = 0;
    _lastAdjustMs = 0;
    _steeringSwWas = readButton(STEERING_SW_PIN); // require steering release before a tap registers
    showAdjusting(driveCenter());
}

VehicleData ResponseTuningScreen::update(int rawX, int rawY) {
    unsigned long now = millis();

    if (_state == State::Confirmed) {
        showConfirmed();
        return buildCommand(driveCenter(), static_cast<uint32_t>(now));
    }

    handleButton(readButton(STEERING_SW_PIN), now);
    if (_state == State::Confirmed) {
        return buildCommand(driveCenter(), static_cast<uint32_t>(now));
    }

    applyAdjust(adjustAxisRaw(rawX, rawY), now);

    int conditioned = conditionedCommand(driveAxisRaw(rawX, rawY));
    showAdjusting(conditioned);
    return buildCommand(conditioned, static_cast<uint32_t>(now));
}

bool ResponseTuningScreen::isComplete() const { return _state == State::Confirmed; }

int ResponseTuningScreen::driveAxisRaw(int rawX, int rawY) const {
    return _axis == TuningAxis::Throttle ? rawY : rawX;
}

int ResponseTuningScreen::adjustAxisRaw(int rawX, int rawY) const {
    return _axis == TuningAxis::Throttle ? rawX : rawY;
}

int ResponseTuningScreen::driveCenter() const {
    return _axis == TuningAxis::Throttle ? THROTTLE_CENTER_RAW : STEERING_CENTER_RAW;
}

int ResponseTuningScreen::adjustCenter() const {
    return _axis == TuningAxis::Throttle ? STEERING_CENTER_RAW : THROTTLE_CENTER_RAW;
}

int ResponseTuningScreen::conditionedCommand(int driveRaw) const {
    if (_axis == TuningAxis::Throttle) {
        return conditionAxis(driveRaw, THROTTLE_CENTER_RAW, THROTTLE_JOY_MIN, THROTTLE_JOY_MAX,
                             _deadzone, _expo, _rate);
    }
    return conditionAxis(driveRaw, STEERING_CENTER_RAW, STEERING_JOY_MIN, STEERING_JOY_MAX,
                         _deadzone, _expo, _rate);
}

// The tuned axis carries the conditioned command; the other axis is held at its
// rest point so only the actuator under test moves.
VehicleData ResponseTuningScreen::buildCommand(int conditionedDrive, uint32_t now) const {
    if (_axis == TuningAxis::Throttle) {
        return VehicleData{STEERING_CENTER_RAW, conditionedDrive, now, 0};
    }
    return VehicleData{conditionedDrive, THROTTLE_CENTER_RAW, now, 0};
}

void ResponseTuningScreen::handleButton(bool steeringSw, unsigned long now) {
    if (steeringSw && !_steeringSwWas) {
        _steeringSwDownAt = now;
        _pressActive = true;
    }
    if (_pressActive && steeringSw && (now - _steeringSwDownAt >= CONFIRM_HOLD_MS)) {
        _pressActive = false;
        confirm();
    } else if (_pressActive && !steeringSw) {
        if (now - _steeringSwDownAt < CONFIRM_HOLD_MS) cycleParameter();
        _pressActive = false;
    }
    _steeringSwWas = steeringSw;
}

void ResponseTuningScreen::cycleParameter() {
    switch (_selected) {
        case Parameter::Deadzone: _selected = Parameter::Expo; break;
        case Parameter::Expo: _selected = Parameter::Rate; break;
        case Parameter::Rate: _selected = Parameter::Deadzone; break;
    }
}

void ResponseTuningScreen::applyAdjust(int adjustRaw, unsigned long now) {
    int deflection = adjustRaw - adjustCenter();
    if (abs(deflection) < ADJUST_THRESHOLD_RAW) return;
    if (now - _lastAdjustMs < ADJUST_INTERVAL_MS) return;
    _lastAdjustMs = now;

    int direction = deflection > 0 ? 1 : -1;
    switch (_selected) {
        case Parameter::Deadzone:
            _deadzone = constrain(_deadzone + direction * DEADZONE_STEP, DEADZONE_MIN, DEADZONE_MAX);
            break;
        case Parameter::Expo:
            _expo = constrain(_expo + direction * CURVE_STEP, 0, RESPONSE_FULL_SCALE);
            break;
        case Parameter::Rate:
            _rate = constrain(_rate + direction * CURVE_STEP, 0, RESPONSE_FULL_SCALE);
            break;
    }
}

void ResponseTuningScreen::confirm() {
    _state = State::Confirmed;
    printResults();
    showConfirmed();
}

void ResponseTuningScreen::showAdjusting(int conditionedDrive) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char line1[24];
    snprintf(line1, sizeof(line1), "%s: %d", parameterName(), selectedValue());

    char line2[48];
    snprintf(line2, sizeof(line2), "DZ%d Ex%d Rt%d  %c:adj hSTR save",
             _deadzone, _expo, _rate, _axis == TuningAxis::Throttle ? 'X' : 'Y');

    char instruction[74];
    snprintf(instruction, sizeof(instruction), "%s\n%s", line1, line2);
    drawCalibrationStep(*driver, axisTitle(), parameterStep(), 3, instruction, conditionedDrive);
}

void ResponseTuningScreen::showConfirmed() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    char line1[24];
    char line2[24];
    if (_axis == TuningAxis::Throttle) {
        snprintf(line1, sizeof(line1), "Expo %d Rate %d", _expo, _rate);
        snprintf(line2, sizeof(line2), "DEADZONE_Y %d", _deadzone);
    } else {
        snprintf(line1, sizeof(line1), "Expo %d Rate %d", _expo, _rate);
        snprintf(line2, sizeof(line2), "DEADZONE_X %d", _deadzone);
    }
    drawCalibrationResult(*driver, axisTitle(), line1, line2, "THR:back - edit cfg");
}

void ResponseTuningScreen::printResults() const {
    if (_axis == TuningAxis::Throttle) {
        debugLogger.log("[FEEL] Throttle - copy into JoystickConfig.h:");
        debugLogger.logf("[FEEL]   #define THROTTLE_EXPO %d", _expo);
        debugLogger.logf("[FEEL]   #define THROTTLE_RATE %d", _rate);
        debugLogger.logf("[FEEL]   #define JOY_DEADZONE_Y %d", _deadzone);
    } else {
        debugLogger.log("[FEEL] Steering - copy into JoystickConfig.h:");
        debugLogger.logf("[FEEL]   #define STEERING_EXPO %d", _expo);
        debugLogger.logf("[FEEL]   #define STEERING_RATE %d", _rate);
        debugLogger.logf("[FEEL]   #define JOY_DEADZONE_X %d", _deadzone);
    }
}

const char* ResponseTuningScreen::axisTitle() const {
    return _axis == TuningAxis::Throttle ? "THROTTLE FEEL" : "STEERING FEEL";
}

const char* ResponseTuningScreen::parameterName() const {
    switch (_selected) {
        case Parameter::Deadzone: return "Deadzone";
        case Parameter::Expo: return "Expo";
        case Parameter::Rate: return "Rate";
    }
    return "";
}

int ResponseTuningScreen::selectedValue() const {
    switch (_selected) {
        case Parameter::Deadzone: return _deadzone;
        case Parameter::Expo: return _expo;
        case Parameter::Rate: return _rate;
    }
    return 0;
}

uint8_t ResponseTuningScreen::parameterStep() const {
    switch (_selected) {
        case Parameter::Deadzone: return 1;
        case Parameter::Expo: return 2;
        case Parameter::Rate: return 3;
    }
    return 1;
}
