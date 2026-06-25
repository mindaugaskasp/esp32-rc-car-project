#include "ServoDriver.h"
#include <config/Esp32Pins.h>
#include <ESP32Servo.h>

Servo servo;

int currentServoAngle = 90;

void initServo() {
    servo.attach(SERVO_PIN);
}

void setServoAngle(int rawX) {
    servo.write(map(rawX, 0, 4095, 0, 180));
}

void updateServo(int rawX) {
    int targetAngle = map(rawX, 0, 4095, 0, 180);
    
    // 2 degrees every loop cycle to even out the servo movement and avoid sudden jumps
    int step = 2; 

    if (currentServoAngle < targetAngle) {
        currentServoAngle += step;
    } else if (currentServoAngle > targetAngle) {
        currentServoAngle -= step;
    }

    // Ensure we don't overshoot the target
    if (abs(currentServoAngle - targetAngle) < step) {
        currentServoAngle = targetAngle;
    }

    servo.write(currentServoAngle);
}