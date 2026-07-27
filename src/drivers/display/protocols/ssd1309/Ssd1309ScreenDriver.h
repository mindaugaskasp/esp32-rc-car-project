#pragma once

#include "drivers/display/protocols/u8g2/U8g2ScreenDriver.h"

// 2.42" SSD1309 128x64 I2C OLED (e.g. Hailege IIC module).
class Ssd1309ScreenDriver : public U8g2ScreenDriver {
public:
    Ssd1309ScreenDriver();
};
