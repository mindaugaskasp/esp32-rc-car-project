#include "JoystickCalibrationScreen.h"
#include "JoystickCalibrationLogic.h"
#include "ui/ScreenUtils.h"
#include "drivers/controls/Controls.h"
#include "drivers/debug/DebugLogger.h"
#include "config/controller/Esp32Pins.h"
#include <Arduino.h>

static const unsigned long RELEASE_MS = 2000;
static const unsigned long CENTER_MS = 1500;

void JoystickCalibrationScreen::begin() {
    _state = State::Release;
    _stateEnteredAt = millis();
    _sumX = _sumY = 0;
    _sampleCount = 0;
    _minX = _minY = ADC_MAX_RAW;
    _maxX = _maxY = 0;
    _restMinX = _restMinY = ADC_MAX_RAW;
    _restMaxX = _restMaxY = 0;
    _sw1Was = readButton(JOY1_SW_PIN); // require SW1 release before it advances a step
    showRelease(2);
}

VehicleData JoystickCalibrationScreen::update(int rawX, int rawY) {
    unsigned long now = millis();
    unsigned long elapsed = now - _stateEnteredAt;
    bool sw1 = readButton(JOY1_SW_PIN);
    bool sw1Pressed = sw1 && !_sw1Was; // rising edge = advance the current sweep step

    switch (_state) {
        case State::Release: {
            int secsLeft = static_cast<int>((RELEASE_MS - min(elapsed, RELEASE_MS)) / 1000) + 1;
            showRelease(secsLeft);
            if (elapsed >= RELEASE_MS) { _state = State::MeasureCenter; _stateEnteredAt = now; }
            break;
        }
        case State::MeasureCenter:
            _sumX += rawX; _sumY += rawY; _sampleCount++;
            // Capture resting jitter per axis (drives the deadzone suggestion), but
            // ignore implausible readings — a stray full-deflection sample from a
            // bump or glitch would otherwise dominate min/max and inflate the deadzone.
            if (isPlausibleRestSample(rawX)) {
                if (rawX < _restMinX) _restMinX = rawX;
                if (rawX > _restMaxX) _restMaxX = rawX;
            }
            if (isPlausibleRestSample(rawY)) {
                if (rawY < _restMinY) _restMinY = rawY;
                if (rawY > _restMaxY) _restMaxY = rawY;
            }
            showMeasuring(constrain(static_cast<int>(static_cast<long>(elapsed) * ADC_MAX_RAW / CENTER_MS), 0, ADC_MAX_RAW));
            if (elapsed >= CENTER_MS) {
                _centerX = computeCenterRaw(_sumX, _sampleCount);
                _centerY = computeCenterRaw(_sumY, _sampleCount);
                _state = State::SweepX;
                _stateEnteredAt = now;
            }
            break;
        case State::SweepX:
            if (rawX < _minX) _minX = rawX;
            if (rawX > _maxX) _maxX = rawX;
            showSweepX(rawX);
            if (sw1Pressed) { _state = State::SweepY; _stateEnteredAt = now; }
            break;
        case State::SweepY:
            if (rawY < _minY) _minY = rawY;
            if (rawY > _maxY) _maxY = rawY;
            showSweepY(rawY);
            if (sw1Pressed) { _state = State::Complete; printResults(); showResult(); }
            break;
        case State::Complete:
            showResult();
            break;
    }
    _sw1Was = sw1;
    return makeNeutralCommand(static_cast<uint32_t>(now));
}

bool JoystickCalibrationScreen::isComplete() const { return _state == State::Complete; }

void JoystickCalibrationScreen::showRelease(int secondsLeft) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char instruction[24];
    snprintf(instruction, sizeof(instruction), "Release sticks: %ds", secondsLeft);
    drawCalibrationStep(*driver,"JOY CALIBRATION", 1, 3, instruction, -1);
}

void JoystickCalibrationScreen::showMeasuring(int progressRaw) {
    ScreenDriver* driver = getScreenDriver();
    if (driver) drawCalibrationStep(*driver,"JOY CALIBRATION", 1, 3, "Measuring center...", progressRaw);
}

void JoystickCalibrationScreen::showSweepX(int rawX) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char instruction[44];
    snprintf(instruction, sizeof(instruction), "Sweep X left<->right\nlo%d hi%d  SW1:next", _minX, _maxX);
    drawCalibrationStep(*driver,"JOY CAL - X AXIS", 2, 3, instruction, rawX);
}

void JoystickCalibrationScreen::showSweepY(int rawY) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char instruction[44];
    snprintf(instruction, sizeof(instruction), "Sweep Y up<->down\nlo%d hi%d  SW1:done", _minY, _maxY);
    drawCalibrationStep(*driver,"JOY CAL - Y AXIS", 3, 3, instruction, rawY);
}

// Result holds until SW2 (CalibrationFlow) so the user can copy these into
// ControlConfig.h. Shows both centers, per-axis travel, and the inferred per-axis
// deadzones; the exact #define lines are also printed to serial by printResults().
void JoystickCalibrationScreen::showResult() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    int deadzoneX = suggestDeadzone(_centerX, _restMinX, _restMaxX);
    int deadzoneY = suggestDeadzone(_centerY, _restMinY, _restMaxY);

    driver->clear();
    driver->font(ScreenFont::Small);
    driver->text(0, 7, "JOY CAL RESULT");
    driver->hline(0, 10, ScreenDriver::W);

    char buffer[26];
    snprintf(buffer, sizeof(buffer), "Xc%d lo%d hi%d", _centerX, _minX, _maxX);
    driver->scrollText(20, buffer);
    snprintf(buffer, sizeof(buffer), "Yc%d lo%d hi%d", _centerY, _minY, _maxY);
    driver->scrollText(30, buffer);
    snprintf(buffer, sizeof(buffer), "Deadzone X%d Y%d", deadzoneX, deadzoneY);
    driver->scrollText(40, buffer);

    driver->font(ScreenFont::Tiny);
    driver->text(0, 62, "SW2:done  cfg in serial log");
    driver->flush();
}

void JoystickCalibrationScreen::printResults() {
    int deadzoneX = suggestDeadzone(_centerX, _restMinX, _restMaxX);
    int deadzoneY = suggestDeadzone(_centerY, _restMinY, _restMaxY);
    debugLogger.logf("[JOY CAL] X center=%d rest[%d..%d] travel[%d..%d]",
                     _centerX, _restMinX, _restMaxX, _minX, _maxX);
    debugLogger.logf("[JOY CAL] Y center=%d rest[%d..%d] travel[%d..%d]",
                     _centerY, _restMinY, _restMaxY, _minY, _maxY);
    debugLogger.log("[JOY CAL] Suggested ControlConfig.h values:");
    debugLogger.logf("[JOY CAL]   #define JOYSTICK_CENTER_RAW %d", _centerX);
    debugLogger.logf("[JOY CAL]   #define THROTTLE_CENTER_RAW %d", _centerY);
    debugLogger.logf("[JOY CAL]   #define JOY_DEADZONE_X %d", deadzoneX);
    debugLogger.logf("[JOY CAL]   #define JOY_DEADZONE_Y %d", deadzoneY);
}
