#include "ScreenDriverFactory.h"
#include "config/ScreenDriverConfig.h"
#include "config/controller/Esp32Pins.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>
#include <Wire.h>

#if ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SSD1306
#include "protocols/ssd1306/Ssd1306ScreenDriver.h"
#elif ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SSD1309
#include "protocols/ssd1309/Ssd1309ScreenDriver.h"
#elif ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SH1106
#include "protocols/sh1106/Sh1106ScreenDriver.h"
#else
#error "ACTIVE_SCREEN_PROTOCOL (config/ScreenDriverConfig.h) is not set to a known SCREEN_PROTOCOL_* value"
#endif

// I2C bus recovery: if power was cut during a previous transmission the
// display may be holding SDA low, blocking all further communication.
// Fix: manually clock SCL 9 times (one full byte + ACK slot) so the stuck
// device finishes its byte and releases SDA, then issue a STOP condition.
// This is the standard recovery procedure from NXP application note AN10116.
static void recoverI2cBus() {
    pinMode(SCREEN_SDA_PIN, INPUT_PULLUP); // let SDA float — don't drive it yet
    pinMode(SCREEN_SCL_PIN, OUTPUT);
    for (int clockPulse = 0; clockPulse < 9; clockPulse++) {
        digitalWrite(SCREEN_SCL_PIN, LOW); delayMicroseconds(5);
        digitalWrite(SCREEN_SCL_PIN, HIGH); delayMicroseconds(5);
    }
    // Issue a STOP condition: SDA goes HIGH while SCL is HIGH.
    pinMode(SCREEN_SDA_PIN, OUTPUT);
    digitalWrite(SCREEN_SDA_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SCREEN_SCL_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCREEN_SDA_PIN, HIGH);
    delayMicroseconds(5);
    // Release both pins so Wire can take them over as hardware I2C.
    pinMode(SCREEN_SCL_PIN, INPUT);
    pinMode(SCREEN_SDA_PIN, INPUT);
}

static constexpr uint8_t OLED_ADDRESS_PRIMARY = 0x3C;
static constexpr uint8_t OLED_ADDRESS_ALTERNATE = 0x3D;

static bool i2cDeviceResponds(uint8_t sevenBitAddress) {
    Wire.beginTransmission(sevenBitAddress);
    return Wire.endTransmission() == 0;
}

ScreenDriver* createScreenDriver() {
    // Recover a stuck bus BEFORE Wire.begin(): recoverI2cBus() leaves the pins as
    // plain INPUT (no pull), so Wire.begin() must run last to (re)enable the
    // internal pull-ups. Otherwise SDA/SCL float low and the S3 I2C peripheral
    // blocks forever waiting for an idle (both-high) bus.
    recoverI2cBus();
    Wire.begin(SCREEN_SDA_PIN, SCREEN_SCL_PIN);
    Wire.setTimeOut(50);  // ms — a stuck bus must not block a probe forever

    // The display is non-critical: probe for it and, if nothing answers, return
    // nullptr so the transmitter boots headless (WiFi/controls still run) instead
    // of hanging in the panel's init. Modules ship at 0x3C or 0x3D.
    uint8_t detectedAddress = 0;
    if (i2cDeviceResponds(OLED_ADDRESS_PRIMARY)) {
        detectedAddress = OLED_ADDRESS_PRIMARY;
    } else if (i2cDeviceResponds(OLED_ADDRESS_ALTERNATE)) {
        detectedAddress = OLED_ADDRESS_ALTERNATE;
    }
    if (detectedAddress == 0) {
        debugLogger.log("[SCREEN] no panel detected - running headless");
        return nullptr;
    }

    // Only the ACTIVE_SCREEN_PROTOCOL driver is compiled, so a single instance
    // exists (function-local static — no heap, one ~1 KB U8G2 framebuffer).
#if ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SSD1306
    static Ssd1306ScreenDriver driver;
#elif ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SSD1309
    static Ssd1309ScreenDriver driver;
#elif ACTIVE_SCREEN_PROTOCOL == SCREEN_PROTOCOL_SH1106
    static Sh1106ScreenDriver driver;
#endif
    if (detectedAddress == OLED_ADDRESS_ALTERNATE) {
        driver.setI2CAddress(OLED_ADDRESS_ALTERNATE);
    }
    // Hand the bus back to U8g2 cleanly: display.begin() calls Wire.begin() again,
    // and the ESP32-S3 I2C driver hangs on a second Wire.begin() unless the first
    // is ended (the classic ESP32 tolerated the double-init; the S3 does not).
    Wire.end();
    return &driver;
}
