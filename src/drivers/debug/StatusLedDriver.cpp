#include "StatusLedDriver.h"
#include "config/controller/Esp32Pins.h"
#include <Arduino.h>

// Low duty on purpose: the WS2812 at full white is bright and a needless current
// draw for a status blink. 40/255 is clearly visible without either downside.
static const uint8_t STATUS_LED_LEVEL = 40;

static StatusColor blinkColor = StatusColor::Off;
static uint16_t blinkPeriodMs = 0;  // 0 = solid (no blink)
static unsigned long lastToggleMs = 0;
static bool blinkPhaseOn = false;

static void writeColor(StatusColor color) {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    switch (color) {
        case StatusColor::Off: break;
        case StatusColor::Red: red = STATUS_LED_LEVEL; break;
        case StatusColor::Green: green = STATUS_LED_LEVEL; break;
        case StatusColor::Blue: blue = STATUS_LED_LEVEL; break;
        case StatusColor::Yellow: red = STATUS_LED_LEVEL; green = STATUS_LED_LEVEL; break;
        case StatusColor::Cyan: green = STATUS_LED_LEVEL; blue = STATUS_LED_LEVEL; break;
        case StatusColor::Magenta: red = STATUS_LED_LEVEL; blue = STATUS_LED_LEVEL; break;
        case StatusColor::White: red = STATUS_LED_LEVEL; green = STATUS_LED_LEVEL; blue = STATUS_LED_LEVEL; break;
    }
    // This board's WS2812 uses RGB byte order while neopixelWrite() assumes GRB,
    // so red and green come out swapped — pass them swapped to compensate.
    neopixelWrite(STATUS_LED_PIN, green, red, blue);
}

void initStatusLed() {
    blinkPeriodMs = 0;
    // WS2812: the first RMT frame after boot is frequently dropped (the data line
    // starts in an undefined state and the peripheral is still warming up), so a
    // lone colour set right after boot never latches. Send a couple of throwaway
    // "off" frames with a gap to prime it; delay is fine here (one-time init).
    writeColor(StatusColor::Off);
    delay(2);
    writeColor(StatusColor::Off);
    delay(2);
}

void setStatusLed(StatusColor color) {
    blinkPeriodMs = 0;
    writeColor(color);
}

void blinkStatusLed(StatusColor color, uint16_t periodMs) {
    blinkColor = color;
    blinkPeriodMs = periodMs;
    lastToggleMs = millis();
    blinkPhaseOn = true;
    writeColor(color);
}

void updateStatusLed() {
    if (blinkPeriodMs == 0) return;  // solid — nothing to advance
    unsigned long now = millis();
    if (now - lastToggleMs < blinkPeriodMs) return;
    lastToggleMs = now;
    blinkPhaseOn = !blinkPhaseOn;
    writeColor(blinkPhaseOn ? blinkColor : StatusColor::Off);
}
