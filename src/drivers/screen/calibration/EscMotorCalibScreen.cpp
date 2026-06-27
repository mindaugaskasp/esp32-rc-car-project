#include "EscMotorCalibScreen.h"
#include "drivers/screen/ScreenUtils.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/controls/Controls.h"
#include "config/Esp32Pins.h"
#include <Arduino.h>

void EscMotorCalibScreen::begin() {
    _state   = State::WaitTest;
    _yWasUp  = _yWasDown = false;
    ScreenDriver* d = getScreenDriver();
    if (d) drawCalibStep(*d,"ESC MOTOR DIR", 1, 2, "ESC ON + armed\nY-up: test forward", -1);
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
    bool sw2         = readButton(JOY2_SW_PIN);
    bool sw2Pressed  = sw2 && !_sw2Was;

    switch (_state) {

        // ── Detection ────────────────────────────────────────────────────────

        case State::WaitTest: {
            if (yUp && !_yWasUp) transitionTo(State::Testing);
            break;
        }

        case State::Testing: {
            int secsLeft = (int)((PULSE_MS - min(elapsed, PULSE_MS)) / 1000) + 1;
            char msg[28];
            snprintf(msg, sizeof(msg), "ESC ON: pulsing\n%ds left", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) drawCalibStep(*d,"ESC MOTOR DIR", 2, 2, msg,
                constrain((int)((long)elapsed * 4095 / PULSE_MS), 0, 4095));
            if (elapsed >= PULSE_MS) {
                transitionTo(State::WaitConfirm);
                ScreenDriver* d2 = getScreenDriver();
                if (d2) drawCalibResult(*d2,"ESC MOTOR DIR",
                    "Did car go FORWARD?", "Y-up = YES", "Y-down = NO");
            }
            _yWasUp = yUp; _yWasDown = yDown;
            return {2048, PULSE_Y};
        }

        case State::WaitConfirm: {
            ScreenDriver* d = getScreenDriver();
            if (d) drawCalibResult(*d,"ESC MOTOR DIR",
                "Did car go FORWARD?", "Y-up = YES", "Y-down = NO");
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC MOTOR] Direction correct.");
                transitionTo(State::ResultOk);
                if (d) drawCalibResult(*d,"MOTOR DIR: OK", "Forward is correct.", nullptr, nullptr);
            }
            if (yDown && !_yWasDown) {
                debugLogger.log("[ESC MOTOR] Reversed — offering ESC programming.");
                transitionTo(State::AskProgram);
                if (d) drawCalibResult(*d,"MOTOR DIR: REVERSED",
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
                if (d) drawCalibResult(*d,"ESC MOTOR PROG",
                    "Turn OFF ESC now.", "SW2 when ready.", nullptr);
            }
            if (yDown && !_yWasDown) {
                debugLogger.log("[ESC MOTOR] Skipped programming. Use THROTTLE_INVERT true.");
                transitionTo(State::SkipProgram);
                ScreenDriver* d = getScreenDriver();
                if (d) drawCalibResult(*d,"SOFTWARE FIX",
                    "Set THROTTLE_INVERT", "true in ControlConfig.h", "then reflash.");
            }
            break;
        }

        case State::SkipProgram:
            break;  // complete

        // ── ESC beep-based programming (user-guided, 4 steps) ────────────────
        // The Dilwe ESC uses a beep-count protocol:
        //   - Power ON with Y=MAX → ESC emits startup beeps, then enters prog mode
        //   - ESC cycles parameters (1 beep = param 1, 2 beeps = param 2, …)
        //   - Pull Y to MIN to select the currently announced parameter
        //   - ESC cycles values (1 beep = value 1, 2 beeps = value 2, …)
        //   - Push Y to MAX to confirm the currently announced value
        //   - ESC emits a confirmation beep sequence; power OFF then ON to apply

        case State::WaitStart: {
            // User must turn ESC off before starting so we can hold Y=MAX on power-on.
            if (sw2Pressed) {
                debugLogger.log("[ESC MOTOR] Step 1/4: holding full throttle — user to power on ESC.");
                transitionTo(State::HoldThrottle);
                ScreenDriver* screenDriver = getScreenDriver();
                if (screenDriver) drawCalibResult(*screenDriver, "ESC PROG 1/4",
                    "Power ON ESC now.",
                    "Wait startup beeps,",
                    "SW2 when in prog mode");
            }
            break;
        }

        case State::HoldThrottle: {
            // Y=MAX is held so the ESC sees full throttle at power-on and enters prog mode.
            // User listens for the startup beep sequence, then presses SW2 to advance.
            if (sw2Pressed) {
                debugLogger.log("[ESC MOTOR] Step 2/4: ESC in prog mode — navigating to motor dir param.");
                transitionTo(State::NavigateToParam);
                ScreenDriver* screenDriver = getScreenDriver();
                if (screenDriver) drawCalibResult(*screenDriver, "ESC PROG 2/4",
                    "Wait 2 beeps:",
                    "= Motor Dir param",
                    "Pull Y to MIN, SW2");
            }
            _yWasUp = yUp; _yWasDown = yDown; _sw2Was = sw2;
            return {2048, 4095};  // hold full throttle — ESC must see MAX at power-on
        }

        case State::NavigateToParam: {
            // Joystick Y is passed through so the user can pull to MIN when they hear 2 beeps.
            // 2 beeps = Motor Direction parameter. Pulling to MIN selects it.
            if (sw2Pressed) {
                debugLogger.log("[ESC MOTOR] Step 3/4: motor dir param selected — selecting reversed value.");
                transitionTo(State::SelectValue);
                ScreenDriver* screenDriver = getScreenDriver();
                if (screenDriver) drawCalibResult(*screenDriver, "ESC PROG 3/4",
                    "Wait 2 beeps:",
                    "= Reversed value",
                    "Push Y to MAX, SW2");
            }
            _yWasUp = yUp; _yWasDown = yDown; _sw2Was = sw2;
            return {2048, rawY};  // pass through Y so user can pull to MIN
        }

        case State::SelectValue: {
            // Joystick Y is passed through so the user can push to MAX when they hear 2 beeps.
            // 2 beeps = Reversed direction value. Pushing to MAX confirms it.
            if (sw2Pressed) {
                debugLogger.log("[ESC MOTOR] Step 4/4: value selected — waiting for confirmation beep.");
                transitionTo(State::WaitConfirmBeep);
                ScreenDriver* screenDriver = getScreenDriver();
                if (screenDriver) drawCalibResult(*screenDriver, "ESC PROG 4/4",
                    "Wait confirm beep.",
                    "Then power OFF ESC.",
                    "SW2 when done");
            }
            _yWasUp = yUp; _yWasDown = yDown; _sw2Was = sw2;
            return {2048, rawY};  // pass through Y so user can push to MAX
        }

        case State::WaitConfirmBeep: {
            // ESC emits a confirmation sequence after the value is accepted.
            // User powers off the ESC, then presses SW2 to finish.
            if (sw2Pressed) {
                debugLogger.log("[ESC MOTOR] Programming complete — user to power cycle ESC and retest.");
                _state = State::ProgramDone;
                ScreenDriver* screenDriver = getScreenDriver();
                if (screenDriver) drawCalibResult(*screenDriver, "ESC PROG DONE",
                    "Power cycle ESC.",
                    "Test direction again",
                    "to confirm fix.");
            }
            break;
        }

        case State::ProgramDone:
            break;  // complete
    }

    _yWasUp = yUp; _yWasDown = yDown; _sw2Was = sw2;
    return {2048, 2048};
}

bool EscMotorCalibScreen::isComplete() const {
    return _state == State::ResultOk      // direction was already correct
        || _state == State::SkipProgram   // user chose THROTTLE_INVERT software fix
        || _state == State::ProgramDone;  // user completed beep-based hardware programming
}
