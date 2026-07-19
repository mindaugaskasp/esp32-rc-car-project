#include "Sh1106ScreenDriver.h"
#include "config/controller/Esp32Pins.h"
#include <U8g2lib.h>

// U8G2_R0 = no rotation; last two args are SCL and SDA pins.
static U8G2_SH1106_128X64_NONAME_F_HW_I2C sh1106Display(U8G2_R0, U8X8_PIN_NONE, SCREEN_SCL_PIN, SCREEN_SDA_PIN);

Sh1106ScreenDriver::Sh1106ScreenDriver() : U8g2ScreenDriver(sh1106Display) {}
