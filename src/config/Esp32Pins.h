#pragma once

const int SERVO_PIN = 32;
const int ESC_PIN = 13;
const int JOY1_X_PIN  = 36;
const int JOY2_Y_PIN  = 39;
const int JOY1_SW_PIN = 26;  // Joystick 1 push-button switch (active LOW, INPUT_PULLUP)
const int JOY2_SW_PIN = 27;  // Joystick 2 push-button switch (active LOW, INPUT_PULLUP)

// Hall effect sensor — receiver only. Connect sensor OUT pin here.
// GPIO 25 supports external interrupts and is free on the receiver board.
const int HALL_SENSOR_PIN = 25;

// I2C pins for the OLED screen (adjust if your wiring differs)
const int SCREEN_SDA_PIN = 21;
const int SCREEN_SCL_PIN = 22;
