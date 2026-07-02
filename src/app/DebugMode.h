#pragma once

// Debug info screen: shows inbound telemetry when it arrives, otherwise the
// raw joystick readings, and forwards joystick input to the car. Mirrors
// DashboardMode's update() contract.
class DebugMode {
public:
    // Returns true if the user pressed a button to open the mode menu (caller
    // should switch to ModeSelect; no joystick data was sent this tick).
    bool update(int joystickX, int joystickY);
};

extern DebugMode debugMode;
