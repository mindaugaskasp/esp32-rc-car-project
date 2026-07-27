#pragma once

#include "drivers/display/ScreenDriver.h"

class U8G2;

// Shared implementation of every drawing primitive on top of a U8G2 display
// object. Protocol subclasses (SH1106, SSD1306) only construct the matching
// U8G2 instance and hand it to this base.
class U8g2ScreenDriver : public ScreenDriver {
public:
    explicit U8g2ScreenDriver(U8G2& display);
    // Override the panel's I2C address (7-bit) before init() — modules ship at
    // 0x3C or 0x3D. No effect once init() has run.
    void setI2CAddress(uint8_t sevenBitAddress);
    void init() override;
    void clear() override;
    void flush() override;
    void font(ScreenFont f) override;
    void text(int x, int y, const char* content) override;
    void scrollText(int y, const char* content) override;
    int textW(const char* content) override;
    void hline(int x, int y, int w) override;
    void vline(int x, int y, int h) override;
    void box(int x, int y, int w, int h) override;
    void frame(int x, int y, int w, int h) override;

private:
    U8G2& display;
};
