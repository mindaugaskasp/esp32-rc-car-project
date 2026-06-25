#include "EscDriver.h"
#include <config/Esp32Pins.h>
#include <config/DebugConfig.h>
#include <ESP32Servo.h>

Servo esc;
int lastProcessedSpeed = 1500;

void initEsc() {
    esc.attach(ESC_PIN, 1000, 2000); 
    esc.writeMicroseconds(1500);
}

void updateEscSpeed(int rawY) {
    // Constrain input to avoid out-of-range map values
    rawY = constrain(rawY, 0, 4095);
    
    int targetSpeed = map(rawY, 0, 4095, 1100, 1900);
    
    // Deadzone logic (already good)
    if (rawY > 1900 && rawY < 2200) targetSpeed = 1500;

    // Incremental movement
    if (abs(lastProcessedSpeed - targetSpeed) > 0) {
        if (lastProcessedSpeed < targetSpeed) {
            lastProcessedSpeed += 5; 
        } else {
            lastProcessedSpeed -= 5;
        }
    }

    esc.writeMicroseconds(lastProcessedSpeed);
    
    DEBUG_LOG_ESC(lastProcessedSpeed);
}