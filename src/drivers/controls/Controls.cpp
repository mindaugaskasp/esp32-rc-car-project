#include "Controls.h"

int readInput(int pin) {
    // Oversample to smooth ADC noise. 16 samples is ~50-100us/axis on the ESP32
    // ADC and removes essentially the same jitter as a much larger burst; higher
    // counts only add per-loop latency to the control path for no real gain.
    long sum = 0;
    const int numSamples = 16;
    for (int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++) {
        sum += analogRead(pin);
    }
    return static_cast<int>(sum / numSamples);
}

int applyAxisInvert(int raw, bool invert, int maxRaw) {
    return invert ? (maxRaw - raw) : raw;
}

void initButton(int pin) {
    pinMode(pin, INPUT_PULLUP);
}

bool readButton(int pin) {
    return digitalRead(pin) == LOW;
}
