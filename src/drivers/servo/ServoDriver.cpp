#include "ServoDriver.h"
#include "ServoLogic.h"
#include <config/Esp32Pins.h>
#include <ESP32Servo.h>

Servo servo;

static int currentServoMicros = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;

void initServo() {
    servo.attach(SERVO_PIN, SERVO_MIN_MICROS, SERVO_MAX_MICROS);
    servo.writeMicroseconds(currentServoMicros);
}

void setServoAngle(int rawX) {
    currentServoMicros = computeServoMicros(rawX);
    servo.writeMicroseconds(currentServoMicros);
}

void updateServo(int rawX) {
    int targetMicros = computeServoMicros(rawX);

    if (targetMicros > currentServoMicros + SERVO_SMOOTHING_STEP_MICROS) {
        currentServoMicros += SERVO_SMOOTHING_STEP_MICROS;
    } else if (targetMicros < currentServoMicros - SERVO_SMOOTHING_STEP_MICROS) {
        currentServoMicros -= SERVO_SMOOTHING_STEP_MICROS;
    } else {
        currentServoMicros = targetMicros;
    }

    servo.writeMicroseconds(currentServoMicros);
}
