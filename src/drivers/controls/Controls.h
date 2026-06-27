#pragma once
#include <Arduino.h>

int  readInput(int pin);
void initButton(int pin);
bool readButton(int pin);  // true while button is pressed (held LOW)