#pragma once

#include <stdint.h>
#include "drivers/wifi/DataTypes.h"

// Combined ESC motor direction calibration: detect first, then optionally program the ESC.
//
// Flow:
//   1. Send a brief forward pulse — user confirms whether the car moved forward.
//   2a. If YES → "Direction OK", done.
//   2b. If NO  → offer to fix via signal-wire programming (no programming card needed).
//       User can accept (programs ESC) or skip (falls back to THROTTLE_INVERT advice).
class EscMotorCalibScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t {
        // --- Detection ---
        WaitTest,       // Y-up to send test pulse
        Testing,        // Brief forward pulse (2 s)
        WaitConfirm,    // "Did car go forward?" Y=yes / Y-dn=no
        ResultOk,       // Direction correct — complete

        // --- Offer to fix ---
        AskProgram,     // "Program ESC to fix?" Y-up=yes / Y-dn=use software flag
        SkipProgram,    // User declined — show THROTTLE_INVERT advice — complete

        // --- ESC signal-wire programming ---
        WaitStart,      // "Turn OFF ESC. Y-up when ready."
        EntryThrottle,  // Y=MAX 5 s — user powers ESC ON during this window
        EntryBrake,     // Y=MIN 3 s — ESC enters programming mode
        SkipParam1,     // Y=MIN 1.5 s — confirm default row 1, advance to row 2
        AdvanceParam2,  // Y=MAX 0.5 s — advance row 2 from value 1 → 2 (Reversal)
        SettleParam2,   // Y=MIN 0.3 s — brief settle
        ConfirmParam2,  // Y=MIN 1.5 s — confirm Reversal, advance to row 3
        ExitProg,       // Y=MAX 3 s  — save confirmed values and exit
        ProgramDone,    // Power cycle ESC — complete
    };

    State _state = State::WaitTest;
    unsigned long _stateEnteredAt = 0;
    bool _yWasUp = false, _yWasDown = false;

    static const int          PULSE_Y          = 2700;
    static const unsigned long PULSE_MS         = 2000;
    static const unsigned long ENTRY_THROTTLE_MS = 5000;
    static const unsigned long ENTRY_BRAKE_MS    = 3000;
    static const unsigned long SKIP_PARAM1_MS    = 1500;
    static const unsigned long ADVANCE_MS        =  500;
    static const unsigned long SETTLE_MS         =  300;
    static const unsigned long CONFIRM_MS        = 1500;
    static const unsigned long EXIT_MS           = 3000;

    void transitionTo(State next);
    void showCountdown(const char* title, unsigned long elapsed, unsigned long total, int sendY);
};
