#pragma once

// ESP32-S3-DevKitC-1 (Waveshare) pin map. Analog inputs MUST stay on ADC1
// (GPIO1-10): ADC2 is unusable while WiFi/ESP-NOW is active. Pins avoided
// board-wide: strapping (0, 3, 45, 46), native USB (19, 20), UART0 console
// (43, 44), SPI flash (26-32), octal PSRAM on the R8 module (33-37), RGB LED (48).

// Joystick ADC inputs — ADC1 channels. Left stick drives throttle (Y, also menu
// up/down navigation), right stick drives steering (X). Each stick's axis + button
// sit on adjacent header pins: throttle on GPIO4/5, steering on GPIO9/10.
const int THROTTLE_Y_PIN = 4; // ADC1_CH3 — left stick, throttle
const int STEERING_X_PIN = 9; // ADC1_CH8 — right stick, steering

// Joystick push-button switches (active LOW, INPUT_PULLUP). Navigation convention:
// STEERING_SW enters/selects/advances (goes deeper), THROTTLE_SW exits/backs out.
const int THROTTLE_SW_PIN = 5; // left stick (throttle) button — exit / back / cancel
const int STEERING_SW_PIN = 10; // right stick (steering) button — enter / select / advance

// Physical DEBUG-mode toggle switch (active LOW, INPUT_PULLUP): wire one leg to
// this pin and the other to GND. While ON it reveals the Debug Info menu entry,
// the dashboard DEBUG MODE badge, and the rotating link-stat readout. GPIO15 is a
// free, non-strapping pin; change it here if you wired the switch elsewhere.
const int DEBUG_MODE_SWITCH_PIN = 15;

// Battery voltage sense — remote battery + through a divider module (values in
// BatteryConfig.h), never directly. GPIO1 is on ADC1 (usable while WiFi/ESP-NOW
// is active). The receiver keeps its own sense pin; only the divider math is shared.
const int BATTERY_SENSE_PIN = 1;

// I2C pins for the OLED screen — remappable via the GPIO matrix; kept adjacent.
const int SCREEN_SDA_PIN = 17;
const int SCREEN_SCL_PIN = 18;

// Onboard WS2812 status LED (diagnostic indicator, see StatusLedDriver). GPIO38 on
// this Waveshare board (the ESP32-S3-DevKitC-1 default is 48); if a future board
// never lights, its WS2812 is wired elsewhere — try 48 or 21.
const int STATUS_LED_PIN = 38;
  