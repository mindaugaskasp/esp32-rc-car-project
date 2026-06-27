#include "Controls.h"


// return avg smooth value
int readInput(int pin) {
    long sum = 0;
    int numSamples = 50;
    for (int i = 0; i < numSamples; i++) {
        sum += analogRead(pin);
    }

    uint smoothedValue = (int) (sum / numSamples);

    return smoothedValue;
}
