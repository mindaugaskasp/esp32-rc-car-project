#pragma once

#include <stdint.h>
#include "drivers/wifi/DataTypes.h"

// Low-voltage cutoff calibration.
//
// User selects: cell count (row 12), per-cell threshold (row 13),
// and protection type (row 14: reduce-power vs. cut-off).
// Then programs rows 12-14 via signal wire; rows 1-11 and 15-17
// are confirmed at position 1 (ESC defaults).
//
// Position mapping assumed (verify against your ESC datasheet):
//   Row 12: 2S=pos1, 3S=pos2, 4S=pos3, 5S=pos4, 6S=pos5
//   Row 13: 2.6V=pos1, 2.7V=pos2, ..., 3.7V=pos12
//   Row 14: Reduce power=pos1, Cut off=pos2
class LowVoltageCalibScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class State : uint8_t {
        // --- Selection ---
        SelectCells,        // Y-tap up/dn to pick cell count; Y-hold 2s to confirm
        SelectThreshold,    // Y-tap up/dn to pick voltage; Y-hold 2s to confirm
        SelectProtection,   // Y-up = reduce power (pos 1); Y-dn = cut off (pos 2)
        // --- ESC signal-wire programming ---
        WaitStart,          // "Turn OFF ESC. Y-up when ready."
        EntryThrottle,      // Y=MAX 5 s — user powers on ESC during this window
        EntryBrake,         // Y=MIN 3 s — ESC enters programming mode
        SkipEarly,          // Y=MIN 11×ROW_MS — confirm defaults for rows 1-11
        Advancing,          // Y=MAX ADVANCE_MS — advance current row one position
        Settling,           // Y=MIN SETTLE_MS  — brief settle between advances
        Confirming,         // Y=MIN ROW_MS     — confirm row at chosen position
        SkipLate,           // Y=MIN 3×ROW_MS   — confirm defaults for rows 15-17
        ExitProg,           // Y=MAX EXIT_MS    — save and exit programming mode
        ProgramDone,        // programming complete
    };

    State   _state          = State::SelectCells;
    uint8_t _cells          = 3;
    uint8_t _thresholdIndex = 4;    // index into THRESHOLDS_DV; 4 = 3.0 V/cell
    bool    _protect        = false; // false = reduce power, true = cut off

    uint8_t       _currentRow   = 12;
    uint8_t       _advancesLeft = 0;
    unsigned long _stateEnteredAt = 0;

    bool _yWasUp = false, _yWasDown = false;
    unsigned long _yUpStart = 0;

    static const unsigned long TAP_MAX_MS        =  800;
    static const unsigned long CONFIRM_HOLD_MS   = 2000;
    static const unsigned long ENTRY_THROTTLE_MS = 5000;
    static const unsigned long ENTRY_BRAKE_MS    = 3000;
    static const unsigned long ROW_MS            = 1500;
    static const unsigned long ADVANCE_MS        =  500;
    static const unsigned long SETTLE_MS         =  300;
    static const unsigned long EXIT_MS           = 3000;
    static const uint8_t       THRESHOLD_COUNT   =   12;

    static const int THRESHOLDS_DV[THRESHOLD_COUNT];

    int  thresholdDv() const;
    void transitionTo(State next);
    void enterNextRow();
    void showCells();
    void showThreshold();
    void showProtection();
    void showResult();
    void printResult();
};
