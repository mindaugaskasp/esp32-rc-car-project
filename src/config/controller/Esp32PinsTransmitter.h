#pragma once

// Joystick ADC inputs — ADC1 channels (ADC1 keeps working while WiFi is active).
// GPIO32/33 are full-featured ADC pins with internal pull resistors, preferred
// over the input-only SENSOR_VP/VN pins (36/39) which have no pulls and are more
// easily disturbed or damaged by overvoltage.
//
// Left stick  — throttle (Y axis, drives the ESC and menu up/down navigation)
// Right stick — steering (X axis, drives the servo)
const int THROTTLE_Y_PIN = 32; // ADC1_CH4 — left stick, throttle
const int STEERING_X_PIN = 33; // ADC1_CH5 — right stick, steering

// Joystick push-button switches (active LOW, INPUT_PULLUP). Navigation convention:
// STEERING_SW enters/selects/advances (goes deeper), THROTTLE_SW exits/backs out.
const int THROTTLE_SW_PIN = 27; // left stick (throttle) button — exit / back / cancel
const int STEERING_SW_PIN = 26; // right stick (steering) button — enter / select / advance

// Physical DEBUG-mode toggle switch (active LOW, INPUT_PULLUP): wire one leg to
// this pin and the other to GND. While ON it reveals the Debug Info menu entry,
// the dashboard DEBUG MODE badge, and the rotating link-stat readout. GPIO25 is a
// free, non-strapping pin with an internal pull-up; change it here if you wired
// the switch elsewhere.
const int DEBUG_MODE_SWITCH_PIN = 25;

// Battery voltage sense — remote battery + through a divider module (values in
// BatteryConfig.h), never directly. GPIO 34 is input-only and on ADC1 (usable
// while WiFi/ESP-NOW is active) — same pin as on the receiver board.
const int BATTERY_SENSE_PIN = 34;

// I2C pins for the OLED screen
const int SCREEN_SDA_PIN = 21;
const int SCREEN_SCL_PIN = 22;
