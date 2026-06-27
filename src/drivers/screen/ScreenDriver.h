#pragma once

#include <stdint.h>

// Font sizes available on all display implementations.
enum class ScreenFont : uint8_t { Tiny, Small, Medium, Large };
// Sh1106 mapping:
//   Tiny   -> u8g2_font_4x6_tf
//   Small  -> u8g2_font_5x7_tf
//   Medium -> u8g2_font_6x10_tf
//   Large  -> u8g2_font_8x13_tf

// Pure primitive drawing surface. No layout logic lives here —
// each screen/view object is responsible for its own rendering.
class ScreenDriver {
public:
    virtual ~ScreenDriver() = default;

    // Hardware init (called once from Screen::begin).
    virtual void init() = 0;

    // Buffer control.
    virtual void clear() = 0;
    virtual void flush() = 0;

    // Text — set font before calling textW() for accurate measurement.
    virtual void font(ScreenFont f) = 0;
    virtual void text(int x, int y, const char* str) = 0;
    virtual void scrollText(int y, const char* str) = 0;  // marquee if too wide
    virtual int  textW(const char* str) = 0;

    // Graphics primitives.
    virtual void hline(int x, int y, int w) = 0;
    virtual void vline(int x, int y, int h) = 0;
    virtual void box(int x, int y, int w, int h) = 0;
    virtual void frame(int x, int y, int w, int h) = 0;

    static const int W = 128;
    static const int H = 64;
};

ScreenDriver* getScreenDriver();
