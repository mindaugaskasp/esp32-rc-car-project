#include "EscRangeCalibScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include <Arduino.h>

static const int RAW_MAX_THRESHOLD  = 3800;
static const int RAW_MIN_THRESHOLD  = 200;
static const int RAW_NEUTRAL_BAND   = 200;
static const unsigned long HOLDMAX_MS = 3000;  // Extra time to power on ESC
static const unsigned long HOLDMIN_MS = 2000;

void EscRangeCalibScreen::begin() {
    _step = EscCalibStep::WaitForMax;
    _stepEnteredAt = millis();
    _detectedMax = _detectedMin = 0;
    showStep(1, 5, "ESC is OFF\nPush Y to MAX", 2048);
}

VehicleData EscRangeCalibScreen::update(int rawX, int rawY) {
    (void)rawX;
    unsigned long now = millis();
    int sendY = rawY;

    switch (_step) {
        case EscCalibStep::WaitForMax: {
            showStep(1, 5, "ESC is OFF\nPush Y to MAX", rawY);
            if (rawY >= RAW_MAX_THRESHOLD) {
                _detectedMax = rawY;
                _step = EscCalibStep::HoldMax;
                _stepEnteredAt = now;
            }
            break;
        }
        case EscCalibStep::HoldMax: {
            unsigned long elapsed = now - _stepEnteredAt;
            if (elapsed >= HOLDMAX_MS) {
                _step = EscCalibStep::WaitForMin;
                _stepEnteredAt = now;
            }
            int secsLeft = (int)((HOLDMAX_MS - min(elapsed, HOLDMAX_MS)) / 1000) + 1;
            char inst[36];
            snprintf(inst, sizeof(inst), "NOW power on ESC\nHolding: %ds left", secsLeft);
            showStep(2, 5, inst, 4095);
            sendY = 4095;  // Full-range max so ESC learns exactly ESC_MAX_MICROS
            break;
        }
        case EscCalibStep::WaitForMin: {
            showStep(3, 5, "ESC is in calib\nPush Y to MIN", rawY);
            if (rawY <= RAW_MIN_THRESHOLD) {
                _detectedMin = rawY;
                _step = EscCalibStep::HoldMin;
                _stepEnteredAt = now;
            }
            break;
        }
        case EscCalibStep::HoldMin: {
            unsigned long elapsed = now - _stepEnteredAt;
            if (elapsed >= HOLDMIN_MS) {
                _step = EscCalibStep::WaitForNeutral;
                _stepEnteredAt = now;
            }
            int secsLeft = (int)((HOLDMIN_MS - min(elapsed, HOLDMIN_MS)) / 1000) + 1;
            char inst[36];
            snprintf(inst, sizeof(inst), "ESC registers MIN\nHold: %ds left", secsLeft);
            showStep(4, 5, inst, 0);
            sendY = 0;  // Full-range min so ESC learns exactly ESC_MIN_MICROS
            break;
        }
        case EscCalibStep::WaitForNeutral: {
            showStep(5, 5, "Release Y to CTR\nESC will arm", rawY);
            if (abs(rawY - 2048) <= RAW_NEUTRAL_BAND) {
                _step = EscCalibStep::Complete;
                showComplete();
            }
            break;
        }
        case EscCalibStep::Complete:
            sendY = 2048;
            showComplete();
            break;
    }
    return {2048, sendY};
}

bool EscRangeCalibScreen::isComplete() const {
    return _step == EscCalibStep::Complete;
}

void EscRangeCalibScreen::showStep(uint8_t stepNum, uint8_t totalSteps, const char* instruction, int rawY) {
    ScreenDriver* d = getScreenDriver();
    if (d) d->displayCalibrationStep("ESC RANGE CAL", stepNum, totalSteps, instruction, rawY);
}

void EscRangeCalibScreen::showComplete() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->displayCalibrationResult("ESC CALIB DONE", "ESC armed & ready!", "Recalibrate if motor", "creeps at neutral.");
}
