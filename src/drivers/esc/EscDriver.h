#pragma once
#include <ESP32Servo.h>

void initEsc();
void updateEscSpeed(int rawY);
void setEscNeutral();
