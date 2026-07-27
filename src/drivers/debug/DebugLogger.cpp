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

void DebugLogger::logJoystick(int joystickX, int joystickY) {
    if (!DEBUG_JOYSTICK_TO_SERIAL) {
        return;
    }
#if DEBUG_LOG_MIN_INTERVAL_MS > 0
    static unsigned long lastLogTime = 0;
    unsigned long now = millis();
    if (now - lastLogTime < DEBUG_LOG_MIN_INTERVAL_MS) {
        return;
    }
    lastLogTime = now;
#endif
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[JOY]: X=%d Y=%d", joystickX, joystickY);
    logSerial(buffer);
}

void DebugLogger::logEsc(int speed) {
    if (!DEBUG_ESC_TO_SERIAL) {
        return;
    }
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[ESC]: %d", speed);
    logSerial(buffer);
}

void DebugLogger::logHallRpm(int rpm) {
    if (!DEBUG_HALL_TO_SERIAL) {
        return;
    }
#if DEBUG_LOG_MIN_INTERVAL_MS > 0
    static unsigned long lastLogTime = 0;
    unsigned long now = millis();
    if (now - lastLogTime < DEBUG_LOG_MIN_INTERVAL_MS) {
        return;
    }
    lastLogTime = now;
#endif
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[HALL]: rpm=%d", rpm);
    logSerial(buffer);
}
