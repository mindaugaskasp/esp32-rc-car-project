#pragma once
#include <stdint.h>

// Onboard WS2812 RGB LED used as a serial-independent diagnostic indicator: set a
// solid colour to mark a boot stage, or blink (non-blocking) to show loop() is
// alive. If nothing lights up, STATUS_LED_PIN is wrong for your board — the WS2812
// sits on different GPIOs across boards (common Waveshare/ESP32-S3 pins: 48, 21, 38).
// LED state -> meaning reference: docs/status-led.md
enum class StatusColor : uint8_t { Off, Red, Green, Blue, Yellow, Cyan, Magenta, White };

void initStatusLed();
void setStatusLed(StatusColor color);                       // solid colour (cancels any blink)
void blinkStatusLed(StatusColor color, uint16_t periodMs);  // start a non-blocking blink
void updateStatusLed();                                     // call every loop() tick to advance the blink
