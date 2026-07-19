#pragma once

#include "drivers/display/protocols/u8g2/U8g2ScreenDriver.h"

// 1.3" SH1106 128x64 I2C OLED.
class Sh1106ScreenDriver : public U8g2ScreenDriver {
public:
    Sh1106ScreenDriver();
};
