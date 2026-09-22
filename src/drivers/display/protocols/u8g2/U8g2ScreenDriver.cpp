#include "U8g2ScreenDriver.h"
#include "config/ScreenDriverConfig.h"
#include <Arduino.h>
#include <U8g2lib.h>

// Applies SCREEN_FONT_SCALE_OFFSET to the requested tier, clamped to the
// available range, so a single config value shifts the whole UI's text size.
static ScreenFont scaledFontTier(ScreenFont requested) {
    constexpr int TIER_MIN = static_cast<int>(ScreenFont::Tiny);
    constexpr int TIER_MAX = static_cast<int>(ScreenFont::Large);
    int tier = static_cast<int>(requested) + SCREEN_FONT_SCALE_OFFSET;
    if (tier < TIER_MIN) tier = TIER_MIN;
    if (tier > TIER_MAX) tier = TIER_MAX;
    return static_cast<ScreenFont>(tier);
}

static const uint8_t* toU8g2Font(ScreenFont fontChoice) {
    switch (scaledFontTier(fontChoice)) {
        case ScreenFont::Tiny: return u8g2_font_4x6_tf;
        case ScreenFont::Small: return u8g2_font_5x7_tf;
        case ScreenFont::Medium: return u8g2_font_6x10_tf;
        case ScreenFont::Large: return u8g2_font_8x13_tf;
    }
    return u8g2_font_5x7_tf;
}

U8g2ScreenDriver::U8g2ScreenDriver(U8G2& display) : display(display) {}

void U8g2ScreenDriver::setI2CAddress(uint8_t sevenBitAddress) {
    display.setI2CAddress(static_cast<uint8_t>(sevenBitAddress << 1));  // U8g2 wants the 8-bit form
}

void U8g2ScreenDriver::init() {
    display.begin();
}

void U8g2ScreenDriver::clear() {
    display.clearBuffer();
}

void U8g2ScreenDriver::flush() {
    display.sendBuffer();
}

void U8g2ScreenDriver::font(ScreenFont fontChoice) {
    display.setFont(toU8g2Font(fontChoice));
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
