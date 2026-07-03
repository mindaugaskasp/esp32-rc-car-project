#pragma once

#include <stdint.h>

// Debug info screen: a set of pages (raw joystick readings, inbound telemetry,
// …) that the user cycles through with SW2. SW1 opens the mode menu. Joystick
// input is forwarded to the car every tick regardless of the visible page.
class DebugMode {
public:
    // Reset to the first page and seed the button edge-detect. Call on mode entry.
    void begin();

    // Returns true if the user pressed SW1 to open the mode menu (caller should
    // switch to ModeSelect; no joystick data was sent this tick).
    bool update(int joystickX, int joystickY);

private:
    // Pages shown in this mode; a short SW2 press advances to the next one,
    // wrapping around. Add an entry here and a case in update() to introduce
    // another page.
    enum class Page : uint8_t { Joystick, Telemetry, PacketTrace };
    static constexpr uint8_t PAGE_COUNT = 3;

    // On the PacketTrace page a long SW2 hold toggles the runtime packet-trace
    // flag (a short press still cycles pages). No other button is free — SW1
    // opens the mode menu and the joystick drives the car.
    static const unsigned long SW2_LONG_PRESS_MS = 600;

    Page _page = Page::Joystick;
    bool _sw1Was = false; // SW1 (throttle stick) = open mode menu
    bool _sw2Was = false; // SW2 (steering stick) = next page / hold-to-toggle
    unsigned long _sw2PressStart = 0; // millis() when SW2 went down
    bool _sw2LongHandled = false; // long-press already acted on this hold
};

extern DebugMode debugMode;
