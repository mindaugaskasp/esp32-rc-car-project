#ifndef CONTROLS_H
#define CONTROLS_H

#include "config/Esp32Pins.h"

int readInput(int pin) {
    int rawValue = analogRead(pin);
    // Map ADC value (0-4095) to servo angle (0-180) or ESC pulse (1000-2000)
    if (pin == JOY1_X_PIN) {
        return map(rawValue, 0, 4095, 0, 180);
    } else if (pin == JOY2_Y_PIN) {
        return map(rawValue, 0, 4095, 1000, 2000);
    }
    return rawValue;
}

#endif
