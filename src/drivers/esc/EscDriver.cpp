#include "EscDriver.h"
#include "EscLogic.h"
#include <config/Esp32Pins.h>
#include <config/ControlConfig.h>
#include "drivers/debug/DebugLogger.h"
#include <ESP32Servo.h>

Servo esc;

static int currentEscMicros = ESC_NEUTRAL_MICROS;

void initEsc() {
    esc.attach(ESC_PIN, 1000, 2000);
    esc.writeMicroseconds(currentEscMicros);
}

void setEscNeutral() {
    currentEscMicros = ESC_NEUTRAL_MICROS;
    esc.writeMicroseconds(currentEscMicros);
    debugLogger.logEsc(currentEscMicros);
}

void updateEscSpeed(int rawY) {
    int targetSpeed = computeEscMicros(rawY);
    if (abs(targetSpeed - currentEscMicros) < ESC_DEADBAND_MICROS) return;
    currentEscMicros = targetSpeed;
    esc.writeMicroseconds(currentEscMicros);
    debugLogger.logEsc(currentEscMicros);
}