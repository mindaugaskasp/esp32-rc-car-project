#pragma once

#include "drivers/screen/ScreenDriver.h"

class Sh1106ScreenDriver : public ScreenDriver {
public:
    void init() override;
    void clear() override;
    void flush() override;
    void font(ScreenFont f) override;
    void text(int x, int y, const char* str) override;
    void scrollText(int y, const char* str) override;
    int  textW(const char* str) override;
    void hline(int x, int y, int w) override;
    void vline(int x, int y, int h) override;
    void box(int x, int y, int w, int h) override;
    void frame(int x, int y, int w, int h) override;
};
