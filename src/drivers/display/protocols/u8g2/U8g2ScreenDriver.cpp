#include "U8g2ScreenDriver.h"
#include <Arduino.h>
#include <U8g2lib.h>

static const uint8_t* toU8g2Font(ScreenFont f) {
    switch (f) {
        case ScreenFont::Tiny: return u8g2_font_4x6_tf;
        case ScreenFont::Small: return u8g2_font_5x7_tf;
        case ScreenFont::Medium: return u8g2_font_6x10_tf;
        case ScreenFont::Large: return u8g2_font_8x13_tf;
    }
    return u8g2_font_5x7_tf;
}

U8g2ScreenDriver::U8g2ScreenDriver(U8G2& display) : display(display) {}

void U8g2ScreenDriver::init() {
    display.begin();
}

void U8g2ScreenDriver::clear() {
    display.clearBuffer();
}

void U8g2ScreenDriver::flush() {
    display.sendBuffer();
}

void U8g2ScreenDriver::font(ScreenFont f) {
    display.setFont(toU8g2Font(f));
}

void U8g2ScreenDriver::text(int x, int y, const char* content) {
    if (content) display.drawStr(x, y, content);
}

// Draws text normally if it fits within the display width; otherwise scrolls
// it left (marquee): 1 s pause at start, then 1 px per 50 ms.
void U8g2ScreenDriver::scrollText(int y, const char* content) {
    if (!content || !*content) return;
    int w = static_cast<int>(display.getStrWidth(content));
    if (w <= W - 1) {
        display.drawStr(0, y, content);
        return;
    }
    const unsigned long STEP_MS = 50UL;
    const int PAUSE_STEPS = 20; // 1 s pause before scrolling
    const int GAP_PX = 20; // silent gap between loops
    int scrollSteps = w - (W - 1) + GAP_PX;
    int totalSteps = PAUSE_STEPS + scrollSteps;
    int phase = static_cast<int>((millis() / STEP_MS) % static_cast<unsigned long>(totalSteps));
    int x = (phase < PAUSE_STEPS) ? 0 : -(phase - PAUSE_STEPS);
    display.drawStr(x, y, content);
}

int U8g2ScreenDriver::textW(const char* content) {
    return content ? static_cast<int>(display.getStrWidth(content)) : 0;
}

void U8g2ScreenDriver::hline(int x, int y, int w) {
    display.drawHLine(x, y, w);
}

void U8g2ScreenDriver::vline(int x, int y, int h) {
    display.drawVLine(x, y, h);
}

void U8g2ScreenDriver::box(int x, int y, int w, int h) {
    display.drawBox(x, y, w, h);
}

void U8g2ScreenDriver::frame(int x, int y, int w, int h) {
    display.drawFrame(x, y, w, h);
}
