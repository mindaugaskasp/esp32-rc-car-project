#include "EscDriver.h"
#include "EscLogic.h"
#include <config/Esp32Pins.h>
#include "drivers/debug/DebugLogger.h"
#include <ESP32Servo.h>

Servo esc;

void initEsc() {
    esc.attach(ESC_PIN, 1000, 2000);
    esc.writeMicroseconds(ESC_NEUTRAL_MICROS);
}

void setEscNeutral() {
    esc.writeMicroseconds(ESC_NEUTRAL_MICROS);
    debugLogger.logEsc(ESC_NEUTRAL_MICROS);
}

void updateEscSpeed(int rawY) {
    int targetSpeed = computeEscMicros(rawY);
    esc.writeMicroseconds(targetSpeed);
    debugLogger.logEsc(targetSpeed);
}