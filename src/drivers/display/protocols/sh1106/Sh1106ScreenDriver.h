#pragma once

#include "drivers/display/ScreenDriver.h"

class Sh1106ScreenDriver : public ScreenDriver {
public:
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
};
