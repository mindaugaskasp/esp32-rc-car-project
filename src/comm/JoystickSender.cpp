#include "JoystickSender.h"
#include "config/ControlConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/radio/EspNowDriver.h"
#include <Arduino.h>

JoystickSender joystickSender;

void JoystickSender::send(int joystickX, int joystickY, int centerRaw, const uint8_t* mac) {
    unsigned long now = millis();

    if (_prevJoyX < 0) {
        _prevJoyX = joystickX; _prevJoyY = joystickY;
        _lastSentX = joystickX; _lastSentY = joystickY;
        _lastSendTime = now; _consecX = _consecY = 0;
    }

    bool movedX = abs(joystickX - _prevJoyX) > JOY_DEADZONE_X;
    bool movedY = abs(joystickY - _prevJoyY) > JOY_DEADZONE_Y;
    bool outsideNeutral = abs(joystickX - centerRaw) > JOY_DEADZONE_X
                       || abs(joystickY - centerRaw) > JOY_DEADZONE_Y;
    bool repeatSend = outsideNeutral && (now - _lastSendTime >= SEND_REPEAT_MS);
    bool changedSinceLastSend = abs(joystickX - _lastSentX) > JOY_DEADZONE_X
                             || abs(joystickY - _lastSentY) > JOY_DEADZONE_Y;
    bool directionChanged = (abs(_lastSentY - centerRaw) > JOY_DEADZONE_Y)
                          && (abs(joystickY  - centerRaw) > JOY_DEADZONE_Y)
                          && ((_lastSentY < centerRaw) != (joystickY < centerRaw));

    if (movedX) _consecX++; else _consecX = 0;
    if (movedY) _consecY++; else _consecY = 0;
    bool quickUpdate = _consecX >= JOY_CONSECUTIVE_THRESHOLD || _consecY >= JOY_CONSECUTIVE_THRESHOLD;

    if (quickUpdate || repeatSend || changedSinceLastSend || directionChanged) {
        debugLogger.logJoystick(joystickX, joystickY);
        _prevJoyX = joystickX; _prevJoyY = joystickY;
        VehicleData data = {joystickX, joystickY, (uint32_t)millis()};
        sendData(data, mac);
        _sentCount++;
        _lastSentX = joystickX; _lastSentY = joystickY;
        _lastSendTime = now;
        _consecX = _consecY = 0;
    }
}
