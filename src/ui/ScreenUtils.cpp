#include "ScreenUtils.h"
#include "drivers/debug/Esp32SysInfo.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

// Baselines shared by every screen header so the title and its underline stay
// locked together; changing one here moves all screens at once.
static constexpr int SCREEN_HEADER_TITLE_BASELINE = 10;
static constexpr int SCREEN_HEADER_RULE_Y = 13;

void drawScreenHeader(ScreenDriver& driver, const char* title) {
    driver.font(ScreenFont::Medium);
    driver.text(0, SCREEN_HEADER_TITLE_BASELINE, title);
    driver.hline(0, SCREEN_HEADER_RULE_Y, ScreenDriver::W);
}

void drawCalibrationStep(ScreenDriver& driver, const char* title, uint8_t step,
                   uint8_t totalSteps, const char* instruction, int barValue) {
    driver.clear();

    driver.font(ScreenFont::Small);
    driver.text(0, 7, title);

    char stepBuffer[14];
    snprintf(stepBuffer, sizeof(stepBuffer), "Step %d of %d", step, totalSteps);
    driver.font(ScreenFont::Medium);
    driver.text(0, 20, stepBuffer);

    driver.font(ScreenFont::Small);
    const char* nl = strchr(instruction, '\n');
    if (nl) {
        char line1[32];
        int len = static_cast<int>(nl - instruction);
        if (len > 31) len = 31;
        memcpy(line1, instruction, len);
        line1[len] = '\0';
        driver.scrollText(32, line1);
        driver.scrollText(41, nl + 1);
        if (barValue >= 0) {
            int barWidth = static_cast<int>(static_cast<long>(constrain(barValue, 0, 4095)) * 126 / 4095);
            driver.frame(0, 55, 128, 8);
            if (barWidth > 0) driver.box(1, 56, barWidth, 6);
        }
    } else {
        driver.scrollText(32, instruction);
        if (barValue >= 0) {
            char rawBuffer[16];
            snprintf(rawBuffer, sizeof(rawBuffer), "Raw: %d", barValue);
            driver.scrollText(44, rawBuffer);
            int barWidth = static_cast<int>(static_cast<long>(constrain(barValue, 0, 4095)) * 126 / 4095);
            driver.frame(0, 55, 128, 8);
            if (barWidth > 0) driver.box(1, 56, barWidth, 6);
        }
    }

    driver.flush();
}

void drawCalibrationResult(ScreenDriver& driver, const char* title,
                     const char* line1, const char* line2, const char* line3) {
    driver.clear();

    driver.font(ScreenFont::Medium);
    driver.text(0, 12, title);

    driver.font(ScreenFont::Small);
    if (line1) driver.scrollText(27, line1);
    if (line2) driver.scrollText(38, line2);
    if (line3) driver.scrollText(50, line3);

    driver.flush();
}

void drawCalibrationMenu(ScreenDriver& driver, const char* above,
                   const char* selected, const char* below) {
    driver.clear();

    char title[22];
    snprintf(title, sizeof(title), "CALIBRATE R:%d%% F:%d%%",
             getRamUsedPercent(), getFlashUsedPercent());
    driver.font(ScreenFont::Small);
    driver.text(0, 7, title);
    driver.hline(0, 10, ScreenDriver::W);

    if (above) driver.scrollText(23, above);
    if (selected) driver.scrollText(34, selected);
    if (below) driver.scrollText(45, below);

    driver.hline(0, 50, ScreenDriver::W);
    driver.font(ScreenFont::Tiny);
    driver.text(0, 58, "Y:nav  STR:enter  THR:exit");

    driver.flush();
}

void drawSysInfoFooter(ScreenDriver& driver, int y) {
    char buffer[26];
    snprintf(buffer, sizeof(buffer), "RAM:%d%%  FSH:%d%%  %lukB",
             getRamUsedPercent(), getFlashUsedPercent(),
             static_cast<unsigned long>(getFreeHeapBytes() / 1024));
    driver.font(ScreenFont::Tiny);
    driver.text(0, y, buffer);
}
