#ifndef ESP32_PINS_H
#define ESP32_PINS_H

// Serial configuration
#define BAUD_RATE 115200

// Servo pins
#define SERVO_PIN 32

// ESC pin
#define ESC_PIN 13

// Joystick pins
#define JOY1_X_PIN 36   // VP - Servo control
#define JOY2_Y_PIN 39   // VN - ESC control

// I2C pins for the OLED screen (adjust if your wiring differs)
#define SCREEN_SDA_PIN 21
#define SCREEN_SCL_PIN 22

#endif
