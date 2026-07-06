#pragma once
#include <stdint.h>

// The scrollable top-level mode menu (Dashboard / Debug / Calibration / WiFi
// Ping) plus the shared "open the menu" button gesture used by every mode.
// Deliberately knows nothing about the other mode classes or TransmitterOperatingMode — it only
// reports selection/cancellation; the caller owns the actual mode switch.
class ModeSelectMenu {
public:
    // Supplies the list of row labels to display. Call once during setup before
    // the menu is first shown. The array must outlive the menu — pass a static/
    // constexpr table owned by the caller.
    void setEntries(const char* const* names, int8_t count);

    void show();

    // Call when the menu opens: seeds the confirm/cancel edge-detect state to the
    // current button reading so a button still held from the open gesture must be
    // released before it registers as select/back (prevents the menu closing on
    // the same click that opened it).
    void onOpen();

    // Call when the menu closes: mirror of onOpen() for the open gesture, so a
    // button still held after select/cancel doesn't immediately re-open the menu
    // in the mode being returned to.
    void onClose();

    // Returns true once when the SW gesture that opens the menu from another
    // mode is detected (stateful edge-detect, shared across all modes).
    bool checkOpenRequest();

    enum class Result : uint8_t { None, Selected, Exit };
    // joystickY drives the up/down tap gesture; throttle=exit (to Dashboard), steering=select.
    Result update(int joystickY);

    int8_t getCursor() const { return _cursor; }
    void setCursor(int8_t cursor) { _cursor = cursor; }

private:
    static const unsigned long MS_TAP_MAX_MS = 700;

    const char* const* _names = nullptr; // caller-owned row labels
    int8_t _count = 0;
    int8_t _cursor = 0;

    bool _yWasUp = false, _yWasDown = false;
    unsigned long _yUpStart = 0, _yDownStart = 0;
    bool _throttleSwWas = false, _steeringSwWas = false;

    // Separate edge-detect state for checkOpenRequest() — tracks button edges
    // in a completely different context (before the menu is open) from _throttleSwWas
    // /_steeringSwWas above (confirm/cancel while the menu is already open).
    bool _openThrottleSwWas = false, _openSteeringSwWas = false;
};

extern ModeSelectMenu modeSelectMenu;
