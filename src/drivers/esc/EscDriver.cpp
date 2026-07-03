#include "EscDriver.h"
#include "EscLogic.h"
#include <config/controller/Esp32Pins.h>
#include <config/ControlConfig.h>
#include "drivers/debug/DebugLogger.h"
#include <ESP32Servo.h>

static Servo esc;

static int currentEscMicros = ESC_NEUTRAL_MICROS;

void initEsc() {
    esc.attach(ESC_PIN, ESC_ATTACH_MIN_MICROS, ESC_ATTACH_MAX_MICROS);
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