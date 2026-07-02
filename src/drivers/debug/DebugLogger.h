#pragma once

#include <Arduino.h>

class DebugLogger {
public:
    void log(const char* message);
    void logf(const char* format, ...);
    void logJoystick(int joystickX, int joystickY);
    void logEsc(int speed);
};

extern DebugLogger debugLogger;
