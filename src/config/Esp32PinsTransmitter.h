#pragma once

// Joystick ADC inputs — input-only ADC1 pins, silkscreened "VP"/"VN" on most dev boards.
//
// JOY1 = RIGHT stick — steering (X axis, drives the servo)
// JOY2 = LEFT  stick — throttle (Y axis, drives the ESC and menu up/down navigation)
const int JOY1_X_PIN  = 36;  // VP — right stick, steering
const int JOY2_Y_PIN  = 39;  // VN — left stick, throttle

// Joystick push-button switches (active LOW, INPUT_PULLUP)
const int JOY1_SW_PIN = 26;  // right stick (steering) button
const int JOY2_SW_PIN = 27;  // left stick (throttle) button

// I2C pins for the OLED screen
const int SCREEN_SDA_PIN = 21;
const int SCREEN_SCL_PIN = 22;
