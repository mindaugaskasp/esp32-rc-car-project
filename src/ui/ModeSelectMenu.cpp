#include "ModeSelectMenu.h"
#include "drivers/display/ScreenDriver.h"
#include "config/controller/Esp32Pins.h"
#include "config/ControlConfig.h"
#include "drivers/controls/Controls.h"
#include <Arduino.h>
#include <stdio.h>

ModeSelectMenu modeSelectMenu;

void ModeSelectMenu::setEntries(const char* const* names, int8_t count) {
    _names = names;
    _count = count;
}

void ModeSelectMenu::onOpen() {
    _throttleSwWas = readButton(THROTTLE_SW_PIN);
    _steeringSwWas = readButton(STEERING_SW_PIN);
}

void ModeSelectMenu::onClose() {
    _openSteeringSwWas = readButton(STEERING_SW_PIN);
}

bool ModeSelectMenu::checkOpenRequest() {
    // Steering (enter/go deeper) opens the menu; throttle is reserved for exit/back.
    bool steeringSw = readButton(STEERING_SW_PIN);
    bool pressed = steeringSw && !_openSteeringSwWas;
    _openSteeringSwWas = steeringSw;
    return pressed;
}

void ModeSelectMenu::show() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver || !_names) return;
    const char* above = (_cursor > 0) ? _names[_cursor - 1] : nullptr;
    const char* below = (_cursor < _count - 1) ? _names[_cursor + 1] : nullptr;
    char selected[32];
    snprintf(selected, sizeof(selected), "> %s", _names[_cursor]);
    driver->clear();
    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "SELECT MODE");
    driver->hline(0, 13, ScreenDriver::W);
    driver->font(ScreenFont::Small);
    if (above) driver->scrollText(26, above);
    driver->scrollText(37, selected);
    if (below) driver->scrollText(48, below);
    driver->hline(0, 52, ScreenDriver::W);
    driver->font(ScreenFont::Tiny);
    driver->text(0, 60, "Y:nav  STR:select  THR:exit");
    driver->flush();
}

ModeSelectMenu::Result ModeSelectMenu::update(int joystickY) {
    unsigned long now = millis();
    bool yUp = joystickY > JOY_GESTURE_UP_RAW;
    bool yDown = joystickY < JOY_GESTURE_DOWN_RAW;
    bool throttleSw = readButton(THROTTLE_SW_PIN);
    bool steeringSw = readButton(STEERING_SW_PIN);

    if (yUp && !_yWasUp) _yUpStart = now;
    if (yDown && !_yWasDown) _yDownStart = now;

    if (!yUp && _yWasUp && (now - _yUpStart < MS_TAP_MAX_MS)) {
        if (_cursor > 0) _cursor--;  // clamp at first item — no wrap
        show();
    }
    if (!yDown && _yWasDown && (now - _yDownStart < MS_TAP_MAX_MS)) {
        if (_cursor < _count - 1) _cursor++;  // clamp at last item — no wrap
        show();
    }

    Result result = Result::None;
    if (steeringSw && !_steeringSwWas) { // steering stick = select/confirm (enter)
        result = Result::Selected;
    }
    if (throttleSw && !_throttleSwWas) { // throttle stick = exit to Dashboard (back)
        result = Result::Exit;
    }

    _yWasUp = yUp; _yWasDown = yDown;
    _throttleSwWas = throttleSw; _steeringSwWas = steeringSw;
    // No joystick send here — menu navigation takes over Y-axis input while menu is open.
    return result;
}
