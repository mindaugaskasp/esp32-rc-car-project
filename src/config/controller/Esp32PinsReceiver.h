#pragma once

// Servo PWM output
const int SERVO_PIN = 32;

// ESC PWM output
const int ESC_PIN = 13;

// Hall effect sensor — connect sensor OUT pin here.
// GPIO 25 supports external interrupts and is free on the receiver board.
const int HALL_SENSOR_PIN = 25;

// Battery voltage sense — battery + through a resistor divider (values in
// BatteryConfig.h), never directly. GPIO 34 is input-only and on ADC1, which
// stays usable while WiFi/ESP-NOW is active (ADC2 does not).
const int BATTERY_SENSE_PIN = 34;
