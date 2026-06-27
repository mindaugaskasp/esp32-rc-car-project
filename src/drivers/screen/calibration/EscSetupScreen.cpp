#include "EscSetupScreen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>

void EscSetupScreen::begin() {
    _state          = State::WaitConfirm;
    _stateEnteredAt = 0;
    _yWasUp         = false;
    ScreenDriver* d = getScreenDriver();
    if (d) d->displayCalibrationResult("ESC FULL SETUP",
        "RESETS all 17 params!", "Run Motor Dir + LV", "after. Y-up=OK");
}

void EscSetupScreen::transitionTo(State next) {
    _state          = next;
    _stateEnteredAt = millis();
}

VehicleData EscSetupScreen::update(int rawX, int rawY) {
    (void)rawX;
    unsigned long now     = millis();
    unsigned long elapsed = now - _stateEnteredAt;
    bool yUp = rawY > 3500;

    switch (_state) {

        case State::WaitConfirm:
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC SETUP] Confirmed. Waiting for ESC off.");
                transitionTo(State::WaitStart);
                ScreenDriver* d = getScreenDriver();
                if (d) d->displayCalibrationResult("ESC FULL SETUP",
                    "Turn OFF ESC now.", "Y-up when ready.", nullptr);
            }
            break;

        case State::WaitStart:
            if (yUp && !_yWasUp) {
                debugLogger.log("[ESC SETUP] Entry throttle — power on ESC now.");
                transitionTo(State::EntryThrottle);
            }
            break;

        case State::EntryThrottle: {
            int secsLeft = (int)((ENTRY_THROTTLE_MS - min(elapsed, ENTRY_THROTTLE_MS)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "POWER ON ESC  %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC FULL SETUP", 1, 4, msg,
                constrain((int)((long)elapsed * 4095 / ENTRY_THROTTLE_MS), 0, 4095));
            if (elapsed >= ENTRY_THROTTLE_MS) transitionTo(State::EntryBrake);
            _yWasUp = yUp;
            return {2048, 4095};
        }

        case State::EntryBrake: {
            int secsLeft = (int)((ENTRY_BRAKE_MS - min(elapsed, ENTRY_BRAKE_MS)) / 1000) + 1;
            char msg[22];
            snprintf(msg, sizeof(msg), "Entering prog %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC FULL SETUP", 2, 4, msg, 0);
            if (elapsed >= ENTRY_BRAKE_MS) {
                debugLogger.log("[ESC SETUP] In programming mode. Confirming all 17 rows...");
                transitionTo(State::Programming);
            }
            _yWasUp = yUp;
            return {2048, 0};
        }

        case State::Programming: {
            // Hold Y=MIN for ROW_COUNT × ROW_MS. Each ROW_MS window confirms the
            // default (position 1) for one row and automatically advances to the next.
            static const unsigned long TOTAL_MS = (unsigned long)ROW_COUNT * ROW_MS;
            uint8_t row = (uint8_t)(min(elapsed, TOTAL_MS - 1) / ROW_MS) + 1;
            int secsLeft = (int)((TOTAL_MS - min(elapsed, TOTAL_MS)) / 1000) + 1;
            char msg[24];
            snprintf(msg, sizeof(msg), "Row %d/17  %ds left", row, secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC FULL SETUP", 3, 4, msg, 0);
            if (elapsed >= TOTAL_MS) {
                debugLogger.log("[ESC SETUP] All rows confirmed. Saving...");
                transitionTo(State::ExitProg);
            }
            _yWasUp = yUp;
            return {2048, 0};
        }

        case State::ExitProg: {
            int secsLeft = (int)((EXIT_MS - min(elapsed, EXIT_MS)) / 1000) + 1;
            char msg[16];
            snprintf(msg, sizeof(msg), "Saving... %ds", secsLeft);
            ScreenDriver* d = getScreenDriver();
            if (d) d->displayCalibrationStep("ESC FULL SETUP", 4, 4, msg, 4095);
            if (elapsed >= EXIT_MS) {
                debugLogger.log("[ESC SETUP] Done. Power cycle ESC, then run Motor Dir + LV calib.");
                _state = State::Complete;
                ScreenDriver* d2 = getScreenDriver();
                if (d2) d2->displayCalibrationResult("ESC SETUP DONE",
                    "Power cycle ESC.", "Run Motor Dir +", "Low Voltage calib.");
            }
            _yWasUp = yUp;
            return {2048, 4095};
        }

        case State::Complete:
            break;
    }

    _yWasUp = yUp;
    return {2048, 2048};
}

bool EscSetupScreen::isComplete() const {
    return _state == State::Complete;
}
