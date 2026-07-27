#include "Ssd1309ScreenDriver.h"
#include "config/ScreenDriverConfig.h"

#if ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SSD1309
#include "config/controller/Esp32Pins.h"
#include <U8g2lib.h>

// U8G2_R0 = no rotation; last two args are SCL and SDA pins. NONAME2 init table —
// this Waveshare-paired 2.42" module hangs on the NONAME0 init sequence; switch
// back to U8G2_SSD1309_128X64_NONAME0_F_HW_I2C only if a panel renders wrong here.
static U8G2_SSD1309_128X64_NONAME2_F_HW_I2C ssd1309Display(U8G2_R0, U8X8_PIN_NONE, SCREEN_SCL_PIN, SCREEN_SDA_PIN);

Ssd1309ScreenDriver::Ssd1309ScreenDriver() : U8g2ScreenDriver(ssd1309Display) {}
#endif
