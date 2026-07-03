#pragma once
#include <Arduino.h>

int readInput(int pin);
void initButton(int pin);
bool readButton(int pin); // true while button is pressed (held LOW)

// Flip an axis reading end-for-end when the stick is mounted reversed.
// Returns (maxRaw - raw) when invert is true, otherwise raw. Pure — no hardware.
int applyAxisInvert(int raw, bool invert, int maxRaw);