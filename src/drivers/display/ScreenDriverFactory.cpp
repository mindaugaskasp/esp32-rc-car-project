#include "ScreenDriverFactory.h"
#include "protocols/sh1106/Sh1106ScreenDriver.h"
#include "protocols/ssd1306/Ssd1306ScreenDriver.h"
#include "config/controller/Esp32Pins.h"
#include <Arduino.h>
#include <Wire.h>

static const uint8_t DISPLAY_I2C_ADDRESS = 0x3C;

// The controller answers a plain I2C read with its status register, whose low
// nibble identifies the chip family (SSD1306 128x64 -> 0x6, 128x32 -> 0x3,
// SH1106 -> 0x8). Undocumented — the SSD1306 datasheet omits I2C reads — but
// consistent across the common modules; anything unrecognized falls back to
// the SH1106 driver, preserving the pre-detection behavior.
static const uint8_t STATUS_CONTROLLER_NIBBLE_MASK = 0x0F;
static const uint8_t SSD1306_128X64_STATUS_NIBBLE = 0x06;
static const uint8_t SSD1306_128X32_STATUS_NIBBLE = 0x03;

// I2C bus recovery: if power was cut during a previous transmission the
// display may be holding SDA low, blocking all further communication.
// Fix: manually clock SCL 9 times (one full byte + ACK slot) so the stuck
// device finishes its byte and releases SDA, then issue a STOP condition.
// This is the standard recovery procedure from NXP application note AN10116.
// Must run before the detection probe below — a stuck bus would NACK the read
// and silently force the SH1106 fallback.
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

static bool isSsd1306Connected() {
    Wire.begin(SCREEN_SDA_PIN, SCREEN_SCL_PIN);
    const uint8_t readLength = 1;
    if (Wire.requestFrom(DISPLAY_I2C_ADDRESS, readLength) != readLength) return false;
    const uint8_t controllerNibble = static_cast<uint8_t>(Wire.read()) & STATUS_CONTROLLER_NIBBLE_MASK;
    return controllerNibble == SSD1306_128X64_STATUS_NIBBLE ||
           controllerNibble == SSD1306_128X32_STATUS_NIBBLE;
}

ScreenDriver* createScreenDriver() {
    recoverI2cBus();
    // Both instances live statically (~1 KB U8G2 framebuffer each) so the pick
    // stays heap-free; the unused one's buffer is idle RAM.
    static Sh1106ScreenDriver sh1106Driver;
    static Ssd1306ScreenDriver ssd1306Driver;
    if (isSsd1306Connected()) {
        Serial.println("[SCREEN] SSD1306 detected");
        return &ssd1306Driver;
    }
    Serial.println("[SCREEN] SH1106 (default)");
    return &sh1106Driver;
}
