#include "ServoDriver.h"
#include "ServoLogic.h"
#include <config/controller/Esp32Pins.h>
#include <ESP32Servo.h>

static Servo servo;

static int currentServoMicros = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;

void initServo() {
    servo.attach(SERVO_PIN, SERVO_MIN_MICROS, SERVO_MAX_MICROS);
    servo.writeMicroseconds(currentServoMicros);
}

void setServoAngle(int rawX) {
    int targetMicros = computeServoMicros(rawX);
    if (abs(targetMicros - currentServoMicros) < SERVO_DEADBAND_MICROS) return;
    currentServoMicros = targetMicros;
    servo.writeMicroseconds(currentServoMicros);
}

void twitchServo() {
    const int centerMicros = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS;
    for (int sweep = 0; sweep < SERVO_TWITCH_COUNT; sweep++) {
        servo.writeMicroseconds(centerMicros + SERVO_TWITCH_OFFSET_MICROS);
        delay(SERVO_TWITCH_HOLD_MS);
        servo.writeMicroseconds(centerMicros - SERVO_TWITCH_OFFSET_MICROS);
        delay(SERVO_TWITCH_HOLD_MS);
    }
    servo.writeMicroseconds(centerMicros);
    currentServoMicros = centerMicros;
}
