#include "EscParamScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>

// Parameter table: rows 1-12 from the ESC datasheet.
// opts[0] = ESC position 1 (factory default for every row).
// Verify option labels against your specific ESC manual.
const EscParamScreen::ParamDef EscParamScreen::PARAMS[PARAM_COUNT] = {
    // Row 1: Operation model
    { "Op. Model", 4,
      { "Fwd+BndBrk", "Fwd+Rev+Inv", "Fwd+PropBrk", "Fwd+Rev+Prop" } },

    // Row 2: Motor rotation direction
    { "Motor Dir", 2,
      { "Normal", "Reversed" } },

    // Row 3: Start mode (acceleration responsiveness, 1=softest)
    { "Start Mode", 10,
      { "L1","L2","L3","L4","L5","L6","L7","L8","L9","L10" } },

    // Row 4: Minimum forward strength
    { "Min Fwd Str", 10,
      { "5%","7%","9%","12%","14%","16%","18%","20%","22%","25%" } },

    // Row 5: Minimum backing (reverse) strength
    { "Min Bck Str", 10,
      { "6%","7%","8%","12%","14%","16%","18%","20%","92%","100%" } },

    // Row 6: Maximum backing (reverse) strength
    { "Max Bck Str", 8,
      { "23%","32%","40%","49%","58%","66%","75%","83%" } },

    // Row 7: Initial braking (drag brake off-throttle)
    { "Init Brake", 10,
      { "0%","5%","11%","16%","22%","27%","33%","38%","44%","50%" } },

    // Row 8: Maximum braking strength
    { "Max Brake", 10,
      { "0%","11%","22%","33%","44%","55%","66%","77%","88%","100%" } },

    // Row 9: Braking force
    { "Brake Force", 10,
      { "0%","11%","22%","33%","44%","55%","66%","77%","88%","100%" } },

    // Row 10: Neutral point range (dead-band around stick centre)
    { "Neutral Rng", 8,
      { "2%","2.3%","2.6%","3.0%","3.3%","3.6%","4.0%","4.3%" } },

    // Row 11: Brake frequency
    { "Brake Freq", 7,
      { "16KHz","8KHz","4KHz","2KHz","500Hz","250Hz","125Hz" } },

    // Row 12: Li battery cell count (Automatic recognition = pos 1)
    { "Li Cells", 6,
      { "Auto","2S","3S","4S","5S","6S" } },
};

void EscParamScreen::transitionTo(State next) {
    _state          = next;
    _stateEnteredAt = millis();
}

void EscParamScreen::begin() {
    _state     = State::Editing;
    _paramIdx  = 0;
    _progParam = 0;
    for (uint8_t i = 0; i < PARAM_COUNT; i++) _values[i] = 0;
    _yWasUp = _yWasDown = _xWasRight = false;
    showEditor();
}

VehicleData EscParamScreen::update(int rawX, int rawY) {
    unsigned long now     = millis();
    unsigned long elapsed = now - _stateEnteredAt;
    bool yUp    = rawY > 3500;
    bool yDown  = rawY < 500;
    bool xRight = rawX > 3500;

    switch (_state) {

        // ── Parameter editor ─────────────────────────────────────────────────

        case State::Editing: {
            if (yUp    && !_yWasUp)    _yUpStart    = now;
            if (yDown  && !_yWasDown)  _yDownStart  = now;
            if (xRight && !_xWasRight) _xRightStart = now;

            uint8_t optCount = PARAMS[_paramIdx].optCount;

            if (!yUp && _yWasUp && (now - _yUpStart < TAP_MAX_MS)) {
                _values[_paramIdx] = (_values[_paramIdx] < optCount - 1)
                    ? _values[_paramIdx] + 1 : 0;
                showEditor();
            }
            if (!yDown && _yWasDown && (now - _yDownStart < TAP_MAX_MS)) {
                _values[_paramIdx] = (_values[_paramIdx] > 0)
                    ? _values[_paramIdx] - 1 : optCount - 1;
                showEditor();
            }
            if (!xRight && _xWasRight && (now - _xRightStart < TAP_MAX_MS)) {
                if (_paramIdx < PARAM_COUNT - 1) {
                    _paramIdx++;
                    showEditor();
                } else {
                    transitionTo(State::ConfirmProg);
                    showConfirm();
                }
            }
            break;
        }

        // ── Confirm before programming ────────────────────────────────────────

        case State::ConfirmProg: {
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC PARAM] Confirmed. Turn off ESC.");
                transitionTo(State::WaitStart);
                ScreenDriver* d = getScreenDriver();
                if (d) d->displayCalibrationResult("ESC PARAM PROG",
                    "Turn OFF ESC now.", "Y-up when ready.", nullptr);
            }
            break;
        }

        case State::WaitStart: {
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC PARAM] Entry throttle — power on ESC now.");
                transitionTo(State::EntryThrottle);
            }
            break;
        }

        // ── Programming sequence ──────────────────────────────────────────────

        case State::EntryThrottle: {
            int secsLeft = (int)((ENTRY_THROTTLE_MS - min(elapsed, ENTRY_THROTTLE_MS)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "POWER ON ESC  %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC PARAM", 1, 4, msg,
                constrain((int)((long)elapsed * 4095 / ENTRY_THROTTLE_MS), 0, 4095));
            if (elapsed >= ENTRY_THROTTLE_MS) transitionTo(State::EntryBrake);
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {2048, 4095};
        }

        case State::EntryBrake: {
            int secsLeft = (int)((ENTRY_BRAKE_MS - min(elapsed, ENTRY_BRAKE_MS)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "Entering prog %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC PARAM", 2, 4, msg, 0);
            if (elapsed >= ENTRY_BRAKE_MS) {
                debugLogger.log("[ESC PARAM] In prog mode. Row 1...");
                _progParam    = 0;
                _advancesLeft = _values[0];
                if (_advancesLeft > 0) transitionTo(State::Advancing);
                else transitionTo(State::Confirming);
            }
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {2048, 0};
        }

        case State::Advancing: {
            char msg[24];
            snprintf(msg, sizeof(msg), "R%d: adv %d left", _progParam + 1, _advancesLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC PARAM", 3, 4, msg, 4095);
            if (elapsed >= ADVANCE_MS) {
                _advancesLeft--;
                if (_advancesLeft > 0) transitionTo(State::Advancing);
                else transitionTo(State::Settling);
            }
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {2048, 4095};
        }

        case State::Settling: {
            if (elapsed >= SETTLE_MS) transitionTo(State::Confirming);
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {2048, 0};
        }

        case State::Confirming: {
            int secsLeft = (int)((ROW_MS - min(elapsed, ROW_MS)) / 1000) + 1;
            char msg[24];
            snprintf(msg, sizeof(msg), "Confirm R%d  %ds", _progParam + 1, secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC PARAM", 3, 4, msg, 0);
            if (elapsed >= ROW_MS) {
                debugLogger.logf("[ESC PARAM] Row %d = pos %d (%s).",
                    _progParam + 1,
                    _values[_progParam] + 1,
                    PARAMS[_progParam].opts[_values[_progParam]]);
                enterNextParam();
            }
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {2048, 0};
        }

        case State::ExitProg: {
            int secsLeft = (int)((EXIT_MS - min(elapsed, EXIT_MS)) / 1000) + 1;
            char msg[16];
            snprintf(msg, sizeof(msg), "Saving... %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC PARAM", 4, 4, msg, 4095);
            if (elapsed >= EXIT_MS) {
                debugLogger.log("[ESC PARAM] Done. LV protection unchanged. Power cycle ESC.");
                _state = State::Done;
                ScreenDriver* d2 = getScreenDriver();
                if (d2) d2->displayCalibrationResult("ESC PARAM DONE",
                    "Rows 1-12 saved.", "LV prot unchanged.", "Power cycle ESC.");
            }
            _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
            return {2048, 4095};
        }

        case State::Done:
            break;
    }

    _yWasUp = yUp; _yWasDown = yDown; _xWasRight = xRight;
    return {2048, 2048};
}

bool EscParamScreen::isComplete() const {
    return _state == State::Done;
}

void EscParamScreen::enterNextParam() {
    _progParam++;
    if (_progParam >= PARAM_COUNT) {
        // All 12 rows confirmed — hold Y=MAX to save and exit.
        transitionTo(State::ExitProg);
        return;
    }
    _advancesLeft = _values[_progParam];
    if (_advancesLeft > 0) transitionTo(State::Advancing);
    else transitionTo(State::Confirming);
}

void EscParamScreen::showEditor() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    const ParamDef& p = PARAMS[_paramIdx];
    char header[22], val[20];
    snprintf(header, sizeof(header), "ESC TUNE %d/%d", _paramIdx + 1, PARAM_COUNT);
    snprintf(val, sizeof(val), "> %s <", p.opts[_values[_paramIdx]]);
    d->displayCalibrationResult(header, p.name, val, "Y:chg  X>:next");
}

void EscParamScreen::showConfirm() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    d->displayCalibrationResult("PROGRAM ESC?",
        "Rows 1-12 only.", "LV prot unchanged.", "Y-up:go  X<:cancel");
}
