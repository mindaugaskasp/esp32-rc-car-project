#pragma once

#include <stdint.h>
#include "drivers/wifi/DataTypes.h"

// ESC motor direction calibration.
//
// Flow:
//   1. Send a brief forward pulse — user confirms whether the car moved forward.
//   2a. If YES → "Direction OK", done.
//   2b. If NO  → offer to fix via ESC beep-based programming or software flag.
//       Hardware fix: holds Y=MAX so ESC sees full throttle on power-on, then
//       user navigates the beep sequence manually and presses Y-up when done.
class EscMotorCalibScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t {
        // --- Detection ---
        WaitTest,           // Y-up to send test pulse
        Testing,            // Brief forward pulse
        WaitConfirm,        // "Did car go forward?" Y-up=yes / Y-dn=no
        ResultOk,           // Direction correct — complete

        // --- Offer to fix ---
        AskProgram,         // "Program ESC to fix?" Y-up=yes / Y-dn=use software flag
        SkipProgram,        // User declined — show THROTTLE_INVERT advice — complete

        // --- ESC beep-based programming (user-guided, 4 steps) ---
        //
        // Dilwe brushless ESC programming protocol:
        //   Step 1 — Power ON with Y=MAX held → ESC emits startup beeps then enters prog mode
        //   Step 2 — ESC cycles parameters; each param announced by N beeps.
        //            Wait for 2 beeps (= Motor Direction param), then pull Y to MIN to select it.
        //   Step 3 — ESC cycles values; each value announced by N beeps.
        //            Wait for 2 beeps (= Reversed direction value), then push Y to MAX to confirm.
        //   Step 4 — ESC emits confirmation beep. Power OFF ESC, then power back ON normally.
        //
        WaitStart,          // "Turn OFF ESC. SW2 when ready."
        HoldThrottle,       // Y=MAX held — "Power ON ESC, wait startup beeps, SW2 when in prog mode"
        NavigateToParam,    // Pass-through Y — "Wait 2 beeps = Motor Dir, pull Y to MIN, SW2 when done"
        SelectValue,        // Pass-through Y — "Wait 2 beeps = Reversed, push Y to MAX, SW2 when done"
        WaitConfirmBeep,    // Neutral Y — "Wait confirm beep, power OFF ESC, SW2 when done"
        ProgramDone,        // Complete — "Power cycle ESC, test direction again"
    };

    State _state = State::WaitTest;
    unsigned long _stateEnteredAt = 0;
    bool _yWasUp = false, _yWasDown = false;
    bool _sw2Was = false;  // JOY2_SW — advance through programming steps

    static const int           PULSE_Y  = 2700;
    static const unsigned long PULSE_MS = 2000;

    void transitionTo(State next);
};
