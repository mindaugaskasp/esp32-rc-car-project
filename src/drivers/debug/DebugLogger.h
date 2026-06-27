#pragma once

#include <Arduino.h>

class DebugLogger {
    public:
        void enableScreenLogging(bool enabled = false);
        void log(const char* message);
        void logf(const char* format, ...);
        void logJoystick(int x, int y);
        void logEsc(int speed);

    private:
        bool screenEnabled = false;
};

extern DebugLogger debugLogger;
