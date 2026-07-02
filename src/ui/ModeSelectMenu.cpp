#include "ModeSelectMenu.h"
#include "drivers/display/ScreenDriver.h"
#include "config/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include <Arduino.h>
#include <stdio.h>

ModeSelectMenu modeSelectMenu;

void ModeSelectMenu::setEntries(const char* const* names, int8_t count) {
    _names = names;
    _count = count;
}

bool ModeSelectMenu::checkOpenRequest() {
    bool joy1Sw = readButton(JOY1_SW_PIN);
    bool joy2Sw = readButton(JOY2_SW_PIN);
    bool pressed = (joy1Sw && !_openJoy1SwWas) || (joy2Sw && !_openJoy2SwWas);
    _openJoy1SwWas = joy1Sw;
    _openJoy2SwWas = joy2Sw;
    return pressed;
}

void ModeSelectMenu::show() {
    ScreenDriver* d = getScreenDriver();
    if (!d || !_names) return;
    const char* above = (_cursor > 0)           ? _names[_cursor - 1] : nullptr;
    const char* below = (_cursor < _count - 1)  ? _names[_cursor + 1] : nullptr;
    char selected[32];
    snprintf(selected, sizeof(selected), "> %s", _names[_cursor]);
    d->clear();
    d->font(ScreenFont::Medium);
    d->text(0, 10, "SELECT MODE");
    d->hline(0, 13, ScreenDriver::W);
    d->font(ScreenFont::Small);
    if (above) d->scrollText(26, above);
    d->scrollText(37, selected);
    if (below) d->scrollText(48, below);
    d->hline(0, 52, ScreenDriver::W);
    d->font(ScreenFont::Tiny);
    d->text(0, 60, "Y:nav  SW2:select  SW1:back");
    d->flush();
}

ModeSelectMenu::Result ModeSelectMenu::update(int joystickY) {
    unsigned long now = millis();
    bool yUp   = joystickY > 3500;
    bool yDown = joystickY < 500;
    bool sw1   = readButton(JOY1_SW_PIN);
    bool sw2   = readButton(JOY2_SW_PIN);

    if (yUp   && !_yWasUp)   _yUpStart   = now;
    if (yDown && !_yWasDown) _yDownStart = now;

    if (!yUp && _yWasUp && (now - _yUpStart < MS_TAP_MAX_MS)) {
        _cursor = (_cursor > 0) ? _cursor - 1 : _count - 1;
        show();
    }
    if (!yDown && _yWasDown && (now - _yDownStart < MS_TAP_MAX_MS)) {
        _cursor = (_cursor < _count - 1) ? _cursor + 1 : 0;
        show();
    }

    Result result = Result::None;
    if (sw2 && !_sw2Was) {  // SW2 (throttle stick) = select/confirm
        result = Result::Selected;
    }
    if (sw1 && !_sw1Was) {  // SW1 (steering stick) = back/cancel
        result = Result::Cancelled;
    }

    _yWasUp = yUp; _yWasDown = yDown;
    _sw1Was = sw1; _sw2Was   = sw2;
    // No joystick send here — menu navigation takes over Y-axis input while menu is open.
    return result;
}
