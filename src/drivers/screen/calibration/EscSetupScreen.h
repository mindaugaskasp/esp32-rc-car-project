#pragma once

#include <stdint.h>
#include "drivers/wifi/DataTypes.h"

// Programs all 17 ESC parameters to project defaults via the signal wire.
//
// All EscConfig default values are position 1 for every row, so the sequence
// is simply 17 consecutive row-confirm pulses (no advance pulses needed).
//
// USE CASE: initial ESC setup on a fresh / factory-reset ESC.
// WARNING: overwrites ALL previously stored ESC parameters, including motor
//          direction. Re-run ESC Motor Dir and Low Voltage calibrations after.
//
// ASSUMPTION: ESC enters programming mode at position 1 for each row.
class EscSetupScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t {
        WaitConfirm,   // Warn user: resets all params
        WaitStart,     // "Turn OFF ESC. Y-up when ready."
        EntryThrottle, // Y=MAX 5 s — power ESC ON during this window
        EntryBrake,    // Y=MIN 3 s — ESC enters programming mode
        Programming,   // Y=MIN, 17 rows × ROW_MS each — confirm default (pos 1) per row
        ExitProg,      // Y=MAX 3 s — save and exit
        Complete,
    };

    State _state = State::WaitConfirm;
    unsigned long _stateEnteredAt = 0;
    bool _yWasUp = false;

    static const unsigned long ENTRY_THROTTLE_MS = 5000;
    static const unsigned long ENTRY_BRAKE_MS    = 3000;
    static const unsigned long ROW_MS            = 1500; // confirm each row's default
    static const unsigned long EXIT_MS           = 3000;
    static const uint8_t       ROW_COUNT         =   17;

    void transitionTo(State next);
};
