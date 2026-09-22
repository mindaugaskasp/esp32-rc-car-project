#pragma once

// ESP32-S3-WROOM-1U pin map. Analog inputs MUST stay on ADC1 (GPIO1-10): ADC2 is
// unusable while WiFi/ESP-NOW is active. Pins avoided board-wide: strapping
// (0, 3, 45, 46), native USB (19, 20), UART0 console (43, 44), SPI flash (26-32),
// octal PSRAM on R8 modules (33-37).

// Servo PWM output
constexpr int SERVO_PIN = 5;

// ESC PWM output
constexpr int ESC_PIN = 6;

// Hall effect sensor — connect sensor OUT pin here. Driven INPUT_PULLUP so a
// disconnected sensor idles HIGH instead of floating into phantom falling edges.
constexpr int HALL_SENSOR_PIN = 4;

// Battery voltage sense — battery + through a resistor divider (values in
// BatteryConfig.h), never directly. GPIO1 is on ADC1, which stays usable while
// WiFi/ESP-NOW is active (ADC2 does not).
constexpr int BATTERY_SENSE_PIN = 1;

// Onboard WS2812 status LED (diagnostic indicator, see StatusLedDriver and
// docs/status-led.md). GPIO48 is the ESP32-S3-DevKitC-1 default; if the LED never
// lights, this board wires the WS2812 elsewhere — try 38 or 21.
constexpr int STATUS_LED_PIN = 48;
