#include "LowVoltageCalibScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>

const int LowVoltageCalibScreen::THRESHOLDS_DV[THRESHOLD_COUNT] = {
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37
};

int LowVoltageCalibScreen::thresholdDv() const {
    return THRESHOLDS_DV[_thresholdIndex];
}

void LowVoltageCalibScreen::transitionTo(State next) {
    _state          = next;
    _stateEnteredAt = millis();
}

void LowVoltageCalibScreen::begin() {
    _state          = State::SelectCells;
    _cells          = 3;
    _thresholdIndex = 4;
    _protect        = false;
    _yWasUp = _yWasDown = false;
    _yUpStart = 0;
    showCells();
}

VehicleData LowVoltageCalibScreen::update(int rawX, int rawY) {
    (void)rawX;
    unsigned long now     = millis();
    unsigned long elapsed = now - _stateEnteredAt;
    bool yUp   = rawY > 3500;
    bool yDown = rawY < 500;

    switch (_state) {

        // ── Selection ────────────────────────────────────────────────────────

        case State::SelectCells: {
            if (yUp  && !_yWasUp)  _yUpStart = now;
            if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS))
                _cells = (_cells < 6) ? _cells + 1 : 2;
            if (!yDown && _yWasDown)
                _cells = (_cells > 2) ? _cells - 1 : 6;
            if (yUp && (now - _yUpStart >= CONFIRM_HOLD_MS)) {
                _yWasUp = false;
                transitionTo(State::SelectThreshold);
                showThreshold();
                break;
            }
            showCells();
            break;
        }

        case State::SelectThreshold: {
            if (yUp  && !_yWasUp)  _yUpStart = now;
            if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS))
                _thresholdIndex = (_thresholdIndex < THRESHOLD_COUNT - 1) ? _thresholdIndex + 1 : 0;
            if (!yDown && _yWasDown)
                _thresholdIndex = (_thresholdIndex > 0) ? _thresholdIndex - 1 : THRESHOLD_COUNT - 1;
            if (yUp && (now - _yUpStart >= CONFIRM_HOLD_MS)) {
                _yWasUp = false;
                transitionTo(State::SelectProtection);
                showProtection();
                break;
            }
            showThreshold();
            break;
        }

        case State::SelectProtection: {
            if (yUp && !_yWasUp) {
                _protect = false;
                debugLogger.logf("[LOW VOLT] %dS @ %d.%dV/cell, reduce-power protection.",
                    _cells, thresholdDv() / 10, thresholdDv() % 10);
                transitionTo(State::WaitStart);
                ScreenDriver* d = getScreenDriver();
                if (d) d->displayCalibrationResult("LOW VOLTAGE PROG",
                    "Turn OFF ESC now.", "Y-up when ready.", nullptr);
            }
            if (yDown && !_yWasDown) {
                _protect = true;
                debugLogger.logf("[LOW VOLT] %dS @ %d.%dV/cell, cut-off protection.",
                    _cells, thresholdDv() / 10, thresholdDv() % 10);
                transitionTo(State::WaitStart);
                ScreenDriver* d = getScreenDriver();
                if (d) d->displayCalibrationResult("LOW VOLTAGE PROG",
                    "Turn OFF ESC now.", "Y-up when ready.", nullptr);
            }
            break;
        }

        // ── ESC signal-wire programming ───────────────────────────────────────

        case State::WaitStart: {
            if (yUp && !_yWasUp) {
                debugLogger.log("[LOW VOLT] Entry throttle — power on ESC now.");
                transitionTo(State::EntryThrottle);
            }
            break;
        }

        case State::EntryThrottle: {
            int secsLeft = (int)((ENTRY_THROTTLE_MS - min(elapsed, ENTRY_THROTTLE_MS)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "POWER ON ESC  %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("LOW VOLT PROG", 1, 5, msg,
                constrain((int)((long)elapsed * 4095 / ENTRY_THROTTLE_MS), 0, 4095));
            if (elapsed >= ENTRY_THROTTLE_MS) transitionTo(State::EntryBrake);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 4095};
        }

        case State::EntryBrake: {
            int secsLeft = (int)((ENTRY_BRAKE_MS - min(elapsed, ENTRY_BRAKE_MS)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "Entering prog %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("LOW VOLT PROG", 2, 5, msg, 0);
            if (elapsed >= ENTRY_BRAKE_MS) {
                debugLogger.log("[LOW VOLT] ESC in prog mode. Skipping rows 1-11...");
                transitionTo(State::SkipEarly);
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};
        }

        case State::SkipEarly: {
            // Confirm defaults for rows 1-11: hold Y=MIN for 11 × ROW_MS.
            const unsigned long skipMs = 11UL * ROW_MS;
            int secsLeft = (int)((skipMs - min(elapsed, skipMs)) / 1000) + 1;
            uint8_t row  = (uint8_t)(min(elapsed, skipMs - 1) / ROW_MS) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "Row %d/11  %ds left", row, secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("LOW VOLT PROG", 3, 5, msg, 0);
            if (elapsed >= skipMs) {
                _currentRow   = 12;
                _advancesLeft = _cells - 2;  // 2S→pos1(0), 3S→pos2(1), ...
                if (_advancesLeft > 0) transitionTo(State::Advancing);
                else transitionTo(State::Confirming);
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};
        }

        case State::Advancing: {
            char msg[22];
            snprintf(msg, sizeof(msg), "Row %d  adv %d left", _currentRow, _advancesLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("LOW VOLT PROG", 4, 5, msg, 4095);
            if (elapsed >= ADVANCE_MS) {
                _advancesLeft--;
                if (_advancesLeft > 0) transitionTo(State::Advancing);
                else transitionTo(State::Settling);
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 4095};
        }

        case State::Settling: {
            if (elapsed >= SETTLE_MS) transitionTo(State::Confirming);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};
        }

        case State::Confirming: {
            int secsLeft = (int)((ROW_MS - min(elapsed, ROW_MS)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "Confirm row %d  %ds", _currentRow, secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("LOW VOLT PROG", 4, 5, msg, 0);
            if (elapsed >= ROW_MS) {
                debugLogger.logf("[LOW VOLT] Row %d confirmed.", _currentRow);
                enterNextRow();
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};
        }

        case State::SkipLate: {
            // Confirm defaults for rows 15-17.
            const unsigned long skipMs = 3UL * ROW_MS;
            int secsLeft = (int)((skipMs - min(elapsed, skipMs)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "Finalising...  %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("LOW VOLT PROG", 5, 5, msg, 0);
            if (elapsed >= skipMs) transitionTo(State::ExitProg);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};
        }

        case State::ExitProg: {
            int secsLeft = (int)((EXIT_MS - min(elapsed, EXIT_MS)) / 1000) + 1;
            char msg[16];
            snprintf(msg, sizeof(msg), "Saving... %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("LOW VOLT PROG", 5, 5, msg, 4095);
            if (elapsed >= EXIT_MS) {
                printResult();
                _state = State::ProgramDone;
                showResult();
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 4095};
        }

        case State::ProgramDone:
            break;
    }

    _yWasUp = yUp; _yWasDown = yDown;
    return {2048, 2048};
}

bool LowVoltageCalibScreen::isComplete() const {
    return _state == State::ProgramDone;
}

void LowVoltageCalibScreen::enterNextRow() {
    _currentRow++;
    if (_currentRow == 13) {
        // Row 13: per-cell threshold. index 0 → pos1 (0 advances), index 4 → pos5 (4 advances).
        _advancesLeft = _thresholdIndex;
    } else if (_currentRow == 14) {
        // Row 14: protection type. reduce-power → pos1 (0), cut-off → pos2 (1).
        _advancesLeft = _protect ? 1 : 0;
    } else {
        // Rows 12-14 done; skip rows 15-17 then exit.
        transitionTo(State::SkipLate);
        return;
    }
    if (_advancesLeft > 0) transitionTo(State::Advancing);
    else transitionTo(State::Confirming);
}

void LowVoltageCalibScreen::showCells() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    char inst[24];
    snprintf(inst, sizeof(inst), "Cells: %dS  Y-tap", _cells);
    d->displayCalibrationStep("LOW VOLT CUTOFF", 1, 3, inst, -1);
}

void LowVoltageCalibScreen::showThreshold() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    int dv = thresholdDv();
    char inst[24];
    snprintf(inst, sizeof(inst), "%d.%dV/cell  Y-tap", dv / 10, dv % 10);
    d->displayCalibrationStep("LOW VOLT CUTOFF", 2, 3, inst, -1);
}

void LowVoltageCalibScreen::showProtection() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->displayCalibrationResult("LV PROTECTION",
        "Y-up: Reduce power", "Y-dn: Cut off", "(Row 14)");
}

void LowVoltageCalibScreen::showResult() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    int dv = thresholdDv(), packDv = _cells * dv;
    char l1[28], l2[28];
    snprintf(l1, sizeof(l1), "%dS %d.%dV=%d.%dV", _cells, dv/10, dv%10, packDv/10, packDv%10);
    snprintf(l2, sizeof(l2), "Prot: %s", _protect ? "cut off" : "reduce pwr");
    d->displayCalibrationResult("LV PROG DONE", l1, l2, "Power cycle ESC.");
}

void LowVoltageCalibScreen::printResult() {
    int dv = thresholdDv(), packDv = _cells * dv;
    debugLogger.logf("[LOW VOLT] Programmed %dS @ %d.%dV/cell = %d.%dV pack cutoff, prot=%s",
        _cells, dv/10, dv%10, packDv/10, packDv%10, _protect ? "cut-off" : "reduce-power");
}
