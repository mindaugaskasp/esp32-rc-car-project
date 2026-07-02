#include "JoystickCalibrationScreen.h"
#include "ui/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>

static const unsigned long RELEASE_MS = 2000;
static const unsigned long CENTER_MS = 1500;
static const unsigned long SWEEP_MS = 5000;

void JoystickCalibrationScreen::begin() {
    _state = State::Release;
    _stateEnteredAt = millis();
    _sumX = _sumY = 0;
    _sampleCount = 0;
    _minX = _minY = 4095;
    _maxX = _maxY = 0;
    showRelease(2);
}

VehicleData JoystickCalibrationScreen::update(int rawX, int rawY) {
    unsigned long now = millis();
    unsigned long elapsed = now - _stateEnteredAt;

    switch (_state) {
        case State::Release: {
            int secsLeft = (int)((RELEASE_MS - min(elapsed, RELEASE_MS)) / 1000) + 1;
            showRelease(secsLeft);
            if (elapsed >= RELEASE_MS) { _state = State::MeasureCenter; _stateEnteredAt = now; }
            break;
        }
        case State::MeasureCenter:
            _sumX += rawX; _sumY += rawY; _sampleCount++;
            showMeasuring(constrain((int)((long)elapsed * 4095 / CENTER_MS), 0, 4095));
            if (elapsed >= CENTER_MS) {
                _centerX = (int)(_sumX / _sampleCount);
                _centerY = (int)(_sumY / _sampleCount);
                _state = State::SweepExtremes;
                _stateEnteredAt = now;
            }
            break;
        case State::SweepExtremes:
            if (rawX < _minX) _minX = rawX;
            if (rawX > _maxX) _maxX = rawX;
            if (rawY < _minY) _minY = rawY;
            if (rawY > _maxY) _maxY = rawY;
            showSweep(rawX, rawY);
            if (elapsed >= SWEEP_MS) { _state = State::Complete; printResults(); showResult(); }
            break;
        case State::Complete:
            showResult();
            break;
    }
    return {2048, 2048};
}

bool JoystickCalibrationScreen::isComplete() const { return _state == State::Complete; }

void JoystickCalibrationScreen::showRelease(int secondsLeft) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char instruction[24];
    snprintf(instruction, sizeof(instruction), "Release sticks: %ds", secondsLeft);
    drawCalibrationStep(*driver,"JOY CALIBRATION", 1, 2, instruction, -1);
}

void JoystickCalibrationScreen::showMeasuring(int progressRaw) {
    ScreenDriver* driver = getScreenDriver();
    if (driver) drawCalibrationStep(*driver,"JOY CALIBRATION", 1, 2, "Measuring center...", progressRaw);
}

void JoystickCalibrationScreen::showSweep(int rawX, int rawY) {
    ScreenDriver* driver = getScreenDriver();
    if (driver) drawCalibrationStep(*driver,"JOY CALIBRATION", 2, 2, "Sweep all corners!", rawY);
    (void)rawX;
}

void JoystickCalibrationScreen::showResult() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;
    char line1[24], line2[24], line3[24];
    snprintf(line1, sizeof(line1), "X ctr=%d %d-%d", _centerX, _minX, _maxX);
    snprintf(line2, sizeof(line2), "Y ctr=%d %d-%d", _centerY, _minY, _maxY);
    int suggestedDeadzone = max(abs(_centerX - _minX), abs(_maxX - _centerX)) / 20;
    snprintf(line3, sizeof(line3), "Suggested DZ: %d", suggestedDeadzone);
    drawCalibrationResult(*driver,"JOY CAL DONE", line1, line2, line3);
}

void JoystickCalibrationScreen::printResults() {
    debugLogger.logf("[JOY CAL] X center=%d min=%d max=%d", _centerX, _minX, _maxX);
    debugLogger.logf("[JOY CAL] Y center=%d min=%d max=%d", _centerY, _minY, _maxY);
    int suggestedDeadzone = max(abs(_centerX - _minX), abs(_maxX - _centerX)) / 20;
    debugLogger.logf("[JOY CAL] Suggested: #define JOY_DEADZONE %d", suggestedDeadzone);
}
