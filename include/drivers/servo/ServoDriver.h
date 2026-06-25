#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include <ESP32Servo.h>
#include "config/Esp32Pins.h"

static Servo servo;

void initServo() {
    servo.attach(SERVO_PIN);
    servo.write(90); // Center position
    Serial.println("Servo initialized");
}

void updateServo(int angle) {
    angle = constrain(angle, 0, 180);
    servo.write(angle);
}

void setServoAngle(int angle) {
    updateServo(angle);
}

#endif
