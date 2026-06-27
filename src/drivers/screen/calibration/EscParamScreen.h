#pragma once

#include <stdint.h>
#include "drivers/wifi/DataTypes.h"

// ESC parameter tuning — programs rows 1-12 via signal wire then exits,
// leaving rows 13-17 (LV protection, throttle stroke, sync rect) unchanged.
//
// The user scrolls through all 12 parameters, picks values, then confirms.
// X-right tap advances through params; Y-up/dn cycles the current value.
//
// Option positions are 0-based in _values[] but map to ESC positions 1-N.
// Values sourced from the 17-parameter ESC datasheet. Verify against yours.
class EscParamScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    static const uint8_t PARAM_COUNT = 12; // rows 1-12

    struct ParamDef {
        const char* name;
        uint8_t     optCount;
        const char* opts[10]; // max 10 options across all 12 rows
    };

    static const ParamDef PARAMS[PARAM_COUNT];

    enum class State : uint8_t {
        Editing,        // user scrolls params and picks values
        ConfirmProg,    // show summary, Y-up to start
        WaitStart,      // "Turn OFF ESC. Y-up when ready."
        EntryThrottle,  // Y=MAX 5 s — user powers on ESC during this window
        EntryBrake,     // Y=MIN 3 s — ESC enters programming mode
        Advancing,      // Y=MAX ADVANCE_MS — advance current row one position
        Settling,       // Y=MIN SETTLE_MS  — brief settle after advance
        Confirming,     // Y=MIN ROW_MS     — confirm row at chosen position
        ExitProg,       // Y=MAX EXIT_MS    — save rows 1-12 and exit
        Done,
    };

    State   _state        = State::Editing;
    uint8_t _paramIdx     = 0;             // param shown in editor (0-11)
    uint8_t _values[PARAM_COUNT] = {};     // selected 0-based option per param
    uint8_t _progParam    = 0;             // param currently being programmed
    uint8_t _advancesLeft = 0;

    unsigned long _stateEnteredAt = 0;

    bool _yWasUp    = false;
    bool _yWasDown  = false;
    bool _xWasRight = false;
    unsigned long _yUpStart    = 0;
    unsigned long _yDownStart  = 0;
    unsigned long _xRightStart = 0;

    static const unsigned long TAP_MAX_MS        =  700;
    static const unsigned long ENTRY_THROTTLE_MS = 5000;
    static const unsigned long ENTRY_BRAKE_MS    = 3000;
    static const unsigned long ROW_MS            = 1500;
    static const unsigned long ADVANCE_MS        =  500;
    static const unsigned long SETTLE_MS         =  300;
    static const unsigned long EXIT_MS           = 3000;

    void transitionTo(State next);
    void enterNextParam();
    void showEditor();
    void showConfirm();
};
