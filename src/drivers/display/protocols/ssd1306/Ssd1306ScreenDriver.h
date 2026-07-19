#pragma once

#include "drivers/display/protocols/u8g2/U8g2ScreenDriver.h"

// 0.96" SSD1306 128x64 I2C OLED.
class Ssd1306ScreenDriver : public U8g2ScreenDriver {
public:
    Ssd1306ScreenDriver();
};
