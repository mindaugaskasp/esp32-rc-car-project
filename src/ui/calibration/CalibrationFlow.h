#pragma once

#include "comm/DataTypes.h"
#include "JoystickCalibScreen.h"
#include "ServoCalibScreen.h"
#include "ThrDeadzoneCalibScreen.h"

// Top-level calibration orchestrator.
// Shows a scrollable menu of all calibration modes.
// Y-tap up/down navigates; X-right tap launches; X-left tap cancels back to menu.
// Returns to the menu automatically 3 s after a calibration completes.
class CalibrationFlow {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    // Returns true (once) when the user taps X-left at the menu root to exit.
    bool wantsExit();

private:
    enum class State : uint8_t { Menu, Running, ResultPause };
    State  _state   = State::Menu;
    int8_t _cursor  = 0;
    int8_t _active  = -1;
    unsigned long _resultPauseStart = 0;

    static const int8_t ITEM_COUNT = 3;
    static const char* const ITEM_NAMES[ITEM_COUNT];
    static const unsigned long SELECT_MS      = 1500;
    static const unsigned long TAP_MAX_MS     = 700;
    static const unsigned long RESULT_PAUSE_MS = 3000;

    JoystickCalibScreen    _joystick;
    ServoCalibScreen       _servoAlign;
    ThrDeadzoneCalibScreen _thrDeadzone;

    // Y gesture state (menu navigation)
    bool _yWasUp   = false;
    bool _yWasDown = false;
    unsigned long _yUpStart   = 0;
    unsigned long _yDownStart = 0;

    // SW button state (enter / exit)
    bool _sw1Was = false;  // JOY1_SW — cancel / exit
    bool _sw2Was = false;  // JOY2_SW — confirm / enter

    VehicleData updateMenu(int rawX, int rawY);
    VehicleData updateRunning(int rawX, int rawY);
    void        launchActive();
    VehicleData dispatchUpdate(int rawX, int rawY);
    bool        dispatchIsComplete();
    void        showMenu();

    bool _wantsExit = false;
};

extern CalibrationFlow calibrationFlow;
