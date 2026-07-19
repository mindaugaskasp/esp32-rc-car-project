#pragma once
#include <ESP32Servo.h>

void initServo();
void setServoAngle(int rawX);

// One-shot physical "link established" indicator: wiggles the steering a few
// times, then settles back to center. Blocking — call only at connection time.
void twitchServo();