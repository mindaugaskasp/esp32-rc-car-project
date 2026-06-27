#include "DebugLogger.h"
#include "config/DebugConfig.h"
#include <stdarg.h>

DebugLogger debugLogger;

static void logSerial(const char* message) {
    unsigned long timestamp = millis();
    char timestamped[192];
    snprintf(timestamped, sizeof(timestamped), "[%lu] %s", timestamp, message);
    Serial.println(timestamped);
}

void DebugLogger::log(const char* message) {
    logSerial(message);
}

void DebugLogger::logf(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    log(buffer);
}

void DebugLogger::logJoystick(int x, int y) {
    if (!DEBUG_JOYSTICK_TO_SERIAL) {
        return;
    }
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[JOY]: X=%d Y=%d", x, y);
    logSerial(buffer);
}

void DebugLogger::logEsc(int speed) {
    if (!CALIBRATION_DEBUG_SCREEN_ENABLED) {
        return;
    }
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[ESC]: %d", speed);
    logSerial(buffer);
}
