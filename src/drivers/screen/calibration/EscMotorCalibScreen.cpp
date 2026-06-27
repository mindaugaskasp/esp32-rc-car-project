#include "EscMotorCalibScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>

void EscMotorCalibScreen::begin() {
    _state   = State::WaitTest;
    _yWasUp  = _yWasDown = false;
    ScreenDriver* d = getScreenDriver();
    if (d) d->displayCalibrationStep("ESC MOTOR DIR", 1, 2, "Y-up to send pulse", -1);
}

void EscMotorCalibScreen::transitionTo(State next) {
    _state          = next;
    _stateEnteredAt = millis();
}

VehicleData EscMotorCalibScreen::update(int rawX, int rawY) {
    (void)rawX;
    unsigned long now     = millis();
    unsigned long elapsed = now - _stateEnteredAt;
    bool yUp   = rawY > 3500;
    bool yDown = rawY < 500;

    switch (_state) {

        // ── Detection ────────────────────────────────────────────────────────

        case State::WaitTest: {
            if (yUp && !_yWasUp) transitionTo(State::Testing);
            break;
        }

        case State::Testing: {
            int secsLeft = (int)((PULSE_MS - min(elapsed, PULSE_MS)) / 1000) + 1;
            char msg[28];
            snprintf(msg, sizeof(msg), "Pulsing... %ds left", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC MOTOR DIR", 2, 2, msg,
                constrain((int)((long)elapsed * 4095 / PULSE_MS), 0, 4095));
            if (elapsed >= PULSE_MS) {
                transitionTo(State::WaitConfirm);
                ScreenDriver* d2 = getScreenDriver();
                if (d2) d2->displayCalibrationResult("ESC MOTOR DIR",
                    "Did car go FORWARD?", "Y-up = YES", "Y-down = NO");
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, PULSE_Y};
        }

        case State::WaitConfirm: {
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationResult("ESC MOTOR DIR",
                "Did car go FORWARD?", "Y-up = YES", "Y-down = NO");
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC MOTOR] Direction correct.");
                transitionTo(State::ResultOk);
                if (d) d->displayCalibrationResult("MOTOR DIR: OK", "Forward is correct.", nullptr, nullptr);
            }
            if (yDown && !_yWasDown) {
                debugLogger.log("[ESC MOTOR] Reversed — offering ESC programming.");
                transitionTo(State::AskProgram);
                if (d) d->displayCalibrationResult("MOTOR DIR: REVERSED",
                    "Program ESC to fix?", "Y-up = YES (no card)", "Y-down = use software");
            }
            break;
        }

        case State::ResultOk:
            break;  // complete, show last screen

        // ── Offer to fix ─────────────────────────────────────────────────────

        case State::AskProgram: {
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC MOTOR] Starting ESC programming sequence.");
                transitionTo(State::WaitStart);
                ScreenDriver* d = getScreenDriver();
                if (d) d->displayCalibrationResult("ESC MOTOR PROG",
                    "Turn OFF ESC now.", "Y-up when ready.", nullptr);
            }
            if (yDown && !_yWasDown) {
                debugLogger.log("[ESC MOTOR] Skipped programming. Use THROTTLE_INVERT true.");
                transitionTo(State::SkipProgram);
                ScreenDriver* d = getScreenDriver();
                if (d) d->displayCalibrationResult("SOFTWARE FIX",
                    "Set THROTTLE_INVERT", "true in ControlConfig.h", "then reflash.");
            }
            break;
        }

        case State::SkipProgram:
            break;  // complete

        // ── ESC signal-wire programming ───────────────────────────────────────

        case State::WaitStart: {
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC MOTOR] Sequence started — hold full throttle.");
                transitionTo(State::EntryThrottle);
            }
            break;
        }

        case State::EntryThrottle:
            showCountdown("NOW POWER ON ESC", elapsed, ENTRY_THROTTLE_MS, 4095);
            if (elapsed >= ENTRY_THROTTLE_MS) transitionTo(State::EntryBrake);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 4095};

        case State::EntryBrake:
            showCountdown("Entering prog mode", elapsed, ENTRY_BRAKE_MS, 0);
            if (elapsed >= ENTRY_BRAKE_MS) transitionTo(State::SkipParam1);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};

        case State::SkipParam1:
            showCountdown("Configuring...", elapsed, SKIP_PARAM1_MS, 0);
            if (elapsed >= SKIP_PARAM1_MS) transitionTo(State::AdvanceParam2);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};

        case State::AdvanceParam2:
            showCountdown("Setting Reversal...", elapsed, ADVANCE_MS, 4095);
            if (elapsed >= ADVANCE_MS) transitionTo(State::SettleParam2);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 4095};

        case State::SettleParam2:
            if (elapsed >= SETTLE_MS) transitionTo(State::ConfirmParam2);
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};

        case State::ConfirmParam2:
            showCountdown("Confirming...", elapsed, CONFIRM_MS, 0);
            if (elapsed >= CONFIRM_MS) {
                debugLogger.log("[ESC MOTOR] Reversal confirmed.");
                transitionTo(State::ExitProg);
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 0};

        case State::ExitProg:
            showCountdown("Saving & exiting...", elapsed, EXIT_MS, 4095);
            if (elapsed >= EXIT_MS) {
                debugLogger.log("[ESC MOTOR] Programming complete. Power cycle ESC.");
                _state = State::ProgramDone;
                ScreenDriver* d = getScreenDriver();
                if (d) d->displayCalibrationResult("ESC PROG DONE",
                    "Power cycle ESC.", "Keep THROTTLE_INVERT", "false.");
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, 4095};

        case State::ProgramDone:
            break;  // complete
    }

    _yWasUp = yUp; _yWasDown = yDown;
    return {2048, 2048};
}

bool EscMotorCalibScreen::isComplete() const {
    return _state == State::ResultOk
        || _state == State::SkipProgram
        || _state == State::ProgramDone;
}

void EscMotorCalibScreen::showCountdown(const char* title, unsigned long elapsed, unsigned long total, int sendY) {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    int secsLeft = (int)((total - min(elapsed, total)) / 1000) + 1;
    char msg[16];
    snprintf(msg, sizeof(msg), "%ds left", secsLeft);
    d->displayCalibrationStep(title, 1, 1, msg, sendY == 4095 ? 4095 : 0);
}
