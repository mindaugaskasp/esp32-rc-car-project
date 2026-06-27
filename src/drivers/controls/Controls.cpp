#include "Controls.h"

int readInput(int pin) {
    long sum = 0;
    const int numSamples = 50;
    for (int i = 0; i < numSamples; i++) {
        sum += analogRead(pin);
    }
    return (int)(sum / numSamples);
}

void initButton(int pin) {
    pinMode(pin, INPUT_PULLUP);
}

bool readButton(int pin) {
    return digitalRead(pin) == LOW;
}
