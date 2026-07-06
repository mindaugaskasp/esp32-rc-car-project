#pragma once

#include "comm/DataTypes.h"
#include "JoystickCalibrationScreen.h"
#include "ServoCalibrationScreen.h"
#include "ResponseTuningScreen.h"

// Top-level calibration orchestrator.
// Shows a scrollable menu of all calibration modes.
// Y-tap up/down navigates; steering launches the selected item; throttle backs out.
// Returns to the menu automatically 3 s after a calibration completes.
class CalibrationFlow {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    // Returns true (once) when the user taps X-left at the menu root to exit.
    bool wantsExit();

private:
    // ResultPause holds the finished calibration's result on-screen until the user
    // presses throttle (back), so they can read/note the reported values before returning.
    enum class State : uint8_t { Menu, Running, ResultPause };
    State _state = State::Menu;
    int8_t _cursor = 0;
    int8_t _active = -1;

    static const int8_t ITEM_COUNT = 4;
    static const char* const ITEM_NAMES[ITEM_COUNT];
    static const unsigned long SELECT_MS = 1500;
    static const unsigned long TAP_MAX_MS = 700;

    JoystickCalibrationScreen _joystick;
    ServoCalibrationScreen _servoAlign;
    ResponseTuningScreen _responseTuning; // shared by the Throttle Feel & Steering Feel items

    // Y gesture state (menu navigation)
    bool _yWasUp = false;
    bool _yWasDown = false;
    unsigned long _yUpStart = 0;
    unsigned long _yDownStart = 0;

    // SW button state (enter / exit)
    bool _throttleSwWas = false; // THROTTLE_SW — exit / back / cancel
    bool _steeringSwWas = false; // STEERING_SW — enter / select / launch

    VehicleData updateMenu(int rawX, int rawY);
    VehicleData updateRunning(int rawX, int rawY);
    void launchActive();
    VehicleData dispatchUpdate(int rawX, int rawY);
    bool dispatchIsComplete();
    void showMenu();

    bool _wantsExit = false;
};

extern CalibrationFlow calibrationFlow;
