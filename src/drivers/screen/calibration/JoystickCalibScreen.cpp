#include "JoystickCalibScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>

static const unsigned long RELEASE_MS = 2000;
static const unsigned long CENTER_MS  = 1500;
static const unsigned long SWEEP_MS   = 5000;

void JoystickCalibScreen::begin() {
    _state = State::Release;
    _stateEnteredAt = millis();
    _sumX = _sumY = 0;
    _sampleCount = 0;
    _minX = _minY = 4095;
    _maxX = _maxY = 0;
    showRelease(2);
}

VehicleData JoystickCalibScreen::update(int rawX, int rawY) {
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

bool JoystickCalibScreen::isComplete() const { return _state == State::Complete; }

void JoystickCalibScreen::showRelease(int secondsLeft) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    char inst[24];
    snprintf(inst, sizeof(inst), "Release sticks: %ds", secondsLeft);
    d->displayCalibrationStep("JOY CALIBRATION", 1, 2, inst, -1);
}

void JoystickCalibScreen::showMeasuring(int progressRaw) {
    ScreenDriver* d = getScreenDriver();
    if (d) d->displayCalibrationStep("JOY CALIBRATION", 1, 2, "Measuring center...", progressRaw);
}

void JoystickCalibScreen::showSweep(int rawX, int rawY) {
    ScreenDriver* d = getScreenDriver();
    if (d) d->displayCalibrationStep("JOY CALIBRATION", 2, 2, "Sweep all corners!", rawY);
    (void)rawX;
}

void JoystickCalibScreen::showResult() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    char l1[24], l2[24], l3[24];
    snprintf(l1, sizeof(l1), "X ctr=%d %d-%d", _centerX, _minX, _maxX);
    snprintf(l2, sizeof(l2), "Y ctr=%d %d-%d", _centerY, _minY, _maxY);
    int dz = max(abs(_centerX - _minX), abs(_maxX - _centerX)) / 20;
    snprintf(l3, sizeof(l3), "Suggested DZ: %d", dz);
    d->displayCalibrationResult("JOY CAL DONE", l1, l2, l3);
}

void JoystickCalibScreen::printResults() {
    debugLogger.logf("[JOY CAL] X center=%d min=%d max=%d", _centerX, _minX, _maxX);
    debugLogger.logf("[JOY CAL] Y center=%d min=%d max=%d", _centerY, _minY, _maxY);
    int dz = max(abs(_centerX - _minX), abs(_maxX - _centerX)) / 20;
    debugLogger.logf("[JOY CAL] Suggested: #define JOY_DEADZONE %d", dz);
}
