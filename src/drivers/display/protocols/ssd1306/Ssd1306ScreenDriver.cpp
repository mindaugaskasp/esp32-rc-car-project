#include "Ssd1306ScreenDriver.h"
#include "config/ScreenDriverConfig.h"

#if ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SSD1306
#include "config/controller/Esp32Pins.h"
#include <U8g2lib.h>

// U8G2_R0 = no rotation; last two args are SCL and SDA pins.
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C ssd1306Display(U8G2_R0, U8X8_PIN_NONE, SCREEN_SCL_PIN, SCREEN_SDA_PIN);

Ssd1306ScreenDriver::Ssd1306ScreenDriver() : U8g2ScreenDriver(ssd1306Display) {}
#endif
