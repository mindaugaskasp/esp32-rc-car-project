#ifndef ESC_DRIVER_H
#define ESC_DRIVER_H

#include <ESP32Servo.h>
#include "config/Esp32Pins.h"

static Servo esc;

void initEsc() {
    esc.attach(ESC_PIN, 1000, 2000);
    esc.writeMicroseconds(1500); // Neutral/stop position
    Serial.println("ESC initialized");
}

void updateEscSpeed(int speed) {
    // Deadzone handling
    if (speed > 1950 && speed < 2050) {
        esc.writeMicroseconds(1500);
    } else {
        speed = constrain(speed, 1000, 2000);
        esc.writeMicroseconds(speed);
    }
}

#endif
