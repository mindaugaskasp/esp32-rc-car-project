#pragma once

// Joystick ADC inputs — ADC1 channels (ADC1 keeps working while WiFi is active).
// GPIO32/33 are full-featured ADC pins with internal pull resistors, preferred
// over the input-only SENSOR_VP/VN pins (36/39) which have no pulls and are more
// easily disturbed or damaged by overvoltage.
//
// JOY1 = LEFT  stick — throttle (Y axis, drives the ESC and menu up/down navigation)
// JOY2 = RIGHT stick — steering (X axis, drives the servo)
const int JOY1_Y_PIN = 32; // ADC1_CH4 — left stick, throttle
const int JOY2_X_PIN = 33; // ADC1_CH5 — right stick, steering

// Joystick push-button switches (active LOW, INPUT_PULLUP)
const int JOY1_SW_PIN = 27; // left stick (throttle) button
const int JOY2_SW_PIN = 26; // right stick (steering) button

// I2C pins for the OLED screen
const int SCREEN_SDA_PIN = 21;
const int SCREEN_SCL_PIN = 22;
