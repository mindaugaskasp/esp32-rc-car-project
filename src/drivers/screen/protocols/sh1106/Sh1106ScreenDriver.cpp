#include <Arduino.h>
#include <U8g2lib.h>
#include "Sh1106ScreenDriver.h"
#include "config/Esp32Pins.h"

// SH1106 128x64 I2C — U8G2_R0 = no rotation; last two args are SCL and SDA pins.
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCREEN_SCL_PIN, SCREEN_SDA_PIN);

// ── Font mapping ──────────────────────────────────────────────────────────────
static const uint8_t* toU8g2Font(ScreenFont f) {
    switch (f) {
        case ScreenFont::Tiny:   return u8g2_font_4x6_tf;
        case ScreenFont::Small:  return u8g2_font_5x7_tf;
        case ScreenFont::Medium: return u8g2_font_6x10_tf;
        case ScreenFont::Large:  return u8g2_font_8x13_tf;
    }
    return u8g2_font_5x7_tf;
}

// ── Primitives ────────────────────────────────────────────────────────────────

void Sh1106ScreenDriver::init() {
    // I2C bus recovery: if power was cut during a previous transmission the
    // display may be holding SDA low, blocking all further communication.
    // Fix: manually clock SCL 9 times (one full byte + ACK slot) so the stuck
    // device finishes its byte and releases SDA, then issue a STOP condition.
    // This is the standard recovery procedure from NXP application note AN10116.
    pinMode(SCREEN_SDA_PIN, INPUT_PULLUP);   // let SDA float — don't drive it yet
    pinMode(SCREEN_SCL_PIN, OUTPUT);
    for (int clockPulse = 0; clockPulse < 9; clockPulse++) {
        digitalWrite(SCREEN_SCL_PIN, LOW);  delayMicroseconds(5);
        digitalWrite(SCREEN_SCL_PIN, HIGH); delayMicroseconds(5);
    }
    // Issue a STOP condition: SDA goes HIGH while SCL is HIGH.
    pinMode(SCREEN_SDA_PIN, OUTPUT);
    digitalWrite(SCREEN_SDA_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SCREEN_SCL_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCREEN_SDA_PIN, HIGH);
    delayMicroseconds(5);
    // Release both pins so U8g2 / Wire can take them over as hardware I2C.
    pinMode(SCREEN_SCL_PIN, INPUT);
    pinMode(SCREEN_SDA_PIN, INPUT);

    u8g2.begin();
}

void Sh1106ScreenDriver::clear() {
    u8g2.clearBuffer();
}

void Sh1106ScreenDriver::flush() {
    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::font(ScreenFont f) {
    u8g2.setFont(toU8g2Font(f));
}

void Sh1106ScreenDriver::text(int x, int y, const char* str) {
    if (str) u8g2.drawStr(x, y, str);
}

// Draws text normally if it fits within the display width; otherwise scrolls
// it left (marquee): 1 s pause at start, then 1 px per 50 ms.
void Sh1106ScreenDriver::scrollText(int y, const char* str) {
    if (!str || !*str) return;
    int w = (int)u8g2.getStrWidth(str);
    if (w <= W - 1) {
        u8g2.drawStr(0, y, str);
        return;
    }
    const unsigned long STEP_MS     = 50UL;
    const int           PAUSE_STEPS = 20;   // 1 s pause before scrolling
    const int           GAP_PX      = 20;   // silent gap between loops
    int scrollSteps = w - (W - 1) + GAP_PX;
    int totalSteps  = PAUSE_STEPS + scrollSteps;
    int phase       = (int)((millis() / STEP_MS) % (unsigned long)totalSteps);
    int x           = (phase < PAUSE_STEPS) ? 0 : -(phase - PAUSE_STEPS);
    u8g2.drawStr(x, y, str);
}

int Sh1106ScreenDriver::textW(const char* str) {
    return str ? (int)u8g2.getStrWidth(str) : 0;
}

void Sh1106ScreenDriver::hline(int x, int y, int w) {
    u8g2.drawHLine(x, y, w);
}

void Sh1106ScreenDriver::vline(int x, int y, int h) {
    u8g2.drawVLine(x, y, h);
}

void Sh1106ScreenDriver::box(int x, int y, int w, int h) {
    u8g2.drawBox(x, y, w, h);
}

void Sh1106ScreenDriver::frame(int x, int y, int w, int h) {
    u8g2.drawFrame(x, y, w, h);
}
