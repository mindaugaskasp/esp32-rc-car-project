#pragma once

#include <stdint.h>

// Debug info screen: a set of pages (raw joystick readings, inbound telemetry,
// …) that the user cycles through with the steering button. Throttle opens the mode menu. Joystick
// input is forwarded to the car every tick regardless of the visible page.
class DebugMode {
public:
    // Reset to the first page and seed the button edge-detect. Call on mode entry.
    void begin();

    // Returns true if the user pressed throttle to open the mode menu (caller should
    // switch to ModeSelect; no joystick data was sent this tick).
    bool update(int joystickX, int joystickY);

private:
    // Pages shown in this mode; a short steering press advances to the next one,
    // wrapping around. Add an entry here and a case in update() to introduce
    // another page.
    enum class Page : uint8_t { Joystick, Telemetry, PacketTrace };
    static constexpr uint8_t PAGE_COUNT = 3;

    // On the PacketTrace page a long steering hold toggles the runtime packet-trace
    // flag (a short press still cycles pages). No other button is free — throttle
    // opens the mode menu and the joystick drives the car.
    static constexpr unsigned long STEERING_SW_LONG_PRESS_MS = 600;

    Page _page = Page::Joystick;
    bool _throttleSwWas = false; // throttle stick = open mode menu (back)
    bool _steeringSwWas = false; // steering stick = next page / hold-to-toggle
    unsigned long _steeringSwPressStart = 0; // millis() when steering went down
    bool _steeringSwLongHandled = false; // long-press already acted on this hold
};

extern DebugMode debugMode;
