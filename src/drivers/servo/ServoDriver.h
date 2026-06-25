#pragma once
#include <ESP32Servo.h>

void initServo();
void setServoAngle(int rawX);
void updateServo(int rawX);