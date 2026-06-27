#include <Arduino.h>
#include <U8g2lib.h>
#include "Sh1106ScreenDriver.h"
#include "config/Esp32Pins.h"

// SH1106 128x64 I2C inicijavimas
// U8G2_R0 reiškia jokio pasukimo, paskutiniai skaičiai - SDA ir SCL pinai
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCREEN_SCL_PIN, SCREEN_SDA_PIN);

static const int JOYSTICK_CENTER_RAW = 2048;

// Draws text normally if it fits, otherwise scrolls it left (marquee).
// 1 s pause at start, then 1 px per 50 ms.
static void drawStr(U8G2_SH1106_128X64_NONAME_F_HW_I2C& g, int y, const char* text) {
    if (!text || !*text) return;
    int w = (int)g.getStrWidth(text);
    if (w <= 127) {
        g.drawStr(0, y, text);
        return;
    }
    const unsigned long STEP_MS    = 50UL;
    const int           PAUSE_STEPS = 20;    // 1 s pause before scrolling
    const int           GAP_PX     = 20;    // silent gap between loops
    int scrollSteps = w - 127 + GAP_PX;
    int totalSteps  = PAUSE_STEPS + scrollSteps;
    int phase       = (int)((millis() / STEP_MS) % (unsigned long)totalSteps);
    int x           = (phase < PAUSE_STEPS) ? 0 : -(phase - PAUSE_STEPS);
    g.drawStr(x, y, text);
}

void Sh1106ScreenDriver::init() {
    u8g2.begin();
    displayStartup("Initializing...");
}

void Sh1106ScreenDriver::displayStartup(const char* message) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, message);
    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::displayConnectionEstablished() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "Starting remote");
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 62, "Connection established");
    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::displayDashboard(float carBatteryVoltage, float remoteBatteryVoltage, int speedRpm, float speedKmh) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 7, "CAR");
    u8g2.drawStr(0, 16, " __");
    u8g2.drawStr(0, 24, "/_o\\");

    char buf[32];
    snprintf(buf, sizeof(buf), "%.2fV", carBatteryVoltage);
    u8g2.drawStr(0, 34, buf);

    u8g2.drawStr(94, 7, "REMOTE");
    u8g2.drawStr(106, 16, "[=]");
    snprintf(buf, sizeof(buf), "%.2fV", remoteBatteryVoltage);
    u8g2.drawStr(94, 26, buf);

    u8g2.setFont(u8g2_font_6x10_tf);
    snprintf(buf, sizeof(buf), "%d RPM", speedRpm);
    int rpmWidth = u8g2.getStrWidth(buf);
    u8g2.drawStr((128 - rpmWidth) / 2, 45, buf);

    snprintf(buf, sizeof(buf), "%.1f km/h", speedKmh);
    int speedWidth = u8g2.getStrWidth(buf);
    u8g2.drawStr((128 - speedWidth) / 2, 60, buf);

    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::displayTelemetry(float batteryVoltage, int speedRpm) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "RC Car Telemetry");

    char buf[32];
    snprintf(buf, sizeof(buf), "Battery: %.2fV", batteryVoltage);
    u8g2.drawStr(0, 30, buf);

    snprintf(buf, sizeof(buf), "Speed: %d RPM", speedRpm);
    u8g2.drawStr(0, 45, buf);

    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::displayDebugJoystick(int joystickX, int joystickY) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 7, "DEBUG JOYSTICK TX");

    char buf[32];
    snprintf(buf, sizeof(buf), "X raw: %d", joystickX);
    u8g2.drawStr(0, 17, buf);

    snprintf(buf, sizeof(buf), "Y raw: %d", joystickY);
    u8g2.drawStr(0, 25, buf);

    snprintf(buf, sizeof(buf), "X delta: %+d", joystickX - JOYSTICK_CENTER_RAW);
    u8g2.drawStr(0, 35, buf);

    snprintf(buf, sizeof(buf), "Y delta: %+d", joystickY - JOYSTICK_CENTER_RAW);
    u8g2.drawStr(0, 43, buf);

    snprintf(buf, sizeof(buf), "Sent: %lu ms", millis());
    u8g2.drawStr(0, 55, buf);

    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::displayCalibrationStep(const char* title, uint8_t step, uint8_t totalSteps, const char* instruction, int barValue) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 7, title);

    char stepBuf[14];
    snprintf(stepBuf, sizeof(stepBuf), "Step %d of %d", step, totalSteps);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 20, stepBuf);

    u8g2.setFont(u8g2_font_5x7_tf);
    const char* nl = strchr(instruction, '\n');
    if (nl) {
        // Two-line instruction: skip "Raw:" label to keep bar visible
        char line1[32];
        int len = (int)(nl - instruction);
        if (len > 31) len = 31;
        memcpy(line1, instruction, len);
        line1[len] = '\0';
        drawStr(u8g2, 32, line1);
        drawStr(u8g2, 41, nl + 1);
        if (barValue >= 0) {
            int barWidth = (int)((long)constrain(barValue, 0, 4095) * 126 / 4095);
            u8g2.drawFrame(0, 55, 128, 8);
            if (barWidth > 0) u8g2.drawBox(1, 56, barWidth, 6);
        }
    } else {
        drawStr(u8g2, 32, instruction);
        if (barValue >= 0) {
            char rawBuf[16];
            snprintf(rawBuf, sizeof(rawBuf), "Raw: %d", barValue);
            drawStr(u8g2, 44, rawBuf);
            int barWidth = (int)((long)constrain(barValue, 0, 4095) * 126 / 4095);
            u8g2.drawFrame(0, 55, 128, 8);
            if (barWidth > 0) u8g2.drawBox(1, 56, barWidth, 6);
        }
    }

    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::displayCalibrationResult(const char* title, const char* line1, const char* line2, const char* line3) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 12, title);

    u8g2.setFont(u8g2_font_5x7_tf);
    if (line1) drawStr(u8g2, 27, line1);
    if (line2) drawStr(u8g2, 38, line2);
    if (line3) drawStr(u8g2, 50, line3);

    u8g2.sendBuffer();
}

void Sh1106ScreenDriver::displayDebugTelemetry(float batteryVoltage, int speedRpm) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 7, "DEBUG TELEMETRY RX");

    char buf[32];
    snprintf(buf, sizeof(buf), "Battery: %.2fV", batteryVoltage);
    u8g2.drawStr(0, 17, buf);

    snprintf(buf, sizeof(buf), "Speed: %d RPM", speedRpm);
    u8g2.drawStr(0, 25, buf);

    snprintf(buf, sizeof(buf), "RX: %lu ms", millis());
    u8g2.drawStr(0, 35, buf);

    u8g2.drawStr(0, 45, "Packet: new hash");

    u8g2.sendBuffer();
}
