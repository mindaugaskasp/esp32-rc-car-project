#include "ScreenUtils.h"
#include "drivers/debug/Esp32SysInfo.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

void drawCalibStep(ScreenDriver& d, const char* title, uint8_t step,
                   uint8_t totalSteps, const char* instruction, int barValue) {
    d.clear();

    d.font(ScreenFont::Small);
    d.text(0, 7, title);

    char stepBuf[14];
    snprintf(stepBuf, sizeof(stepBuf), "Step %d of %d", step, totalSteps);
    d.font(ScreenFont::Medium);
    d.text(0, 20, stepBuf);

    d.font(ScreenFont::Small);
    const char* nl = strchr(instruction, '\n');
    if (nl) {
        char line1[32];
        int len = (int)(nl - instruction);
        if (len > 31) len = 31;
        memcpy(line1, instruction, len);
        line1[len] = '\0';
        d.scrollText(32, line1);
        d.scrollText(41, nl + 1);
        if (barValue >= 0) {
            int barWidth = (int)((long)constrain(barValue, 0, 4095) * 126 / 4095);
            d.frame(0, 55, 128, 8);
            if (barWidth > 0) d.box(1, 56, barWidth, 6);
        }
    } else {
        d.scrollText(32, instruction);
        if (barValue >= 0) {
            char rawBuf[16];
            snprintf(rawBuf, sizeof(rawBuf), "Raw: %d", barValue);
            d.scrollText(44, rawBuf);
            int barWidth = (int)((long)constrain(barValue, 0, 4095) * 126 / 4095);
            d.frame(0, 55, 128, 8);
            if (barWidth > 0) d.box(1, 56, barWidth, 6);
        }
    }

    d.flush();
}

void drawCalibResult(ScreenDriver& d, const char* title,
                     const char* line1, const char* line2, const char* line3) {
    d.clear();

    d.font(ScreenFont::Medium);
    d.text(0, 12, title);

    d.font(ScreenFont::Small);
    if (line1) d.scrollText(27, line1);
    if (line2) d.scrollText(38, line2);
    if (line3) d.scrollText(50, line3);

    d.flush();
}

void drawCalibMenu(ScreenDriver& d, const char* above,
                   const char* selected, const char* below) {
    d.clear();

    char title[22];
    snprintf(title, sizeof(title), "CALIBRATE R:%d%% F:%d%%",
             getRamUsedPercent(), getFlashUsedPercent());
    d.font(ScreenFont::Small);
    d.text(0, 7, title);
    d.hline(0, 10, ScreenDriver::W);

    if (above)    d.scrollText(23, above);
    if (selected) d.scrollText(34, selected);
    if (below)    d.scrollText(45, below);

    d.hline(0, 50, ScreenDriver::W);
    d.font(ScreenFont::Tiny);
    d.text(0, 58, "Y:nav  SW2:enter  SW1:exit");

    d.flush();
}

void drawSysInfoFooter(ScreenDriver& d, int y) {
    char buf[26];
    snprintf(buf, sizeof(buf), "RAM:%d%%  FSH:%d%%  %lukB",
             getRamUsedPercent(), getFlashUsedPercent(),
             (unsigned long)(getFreeHeapBytes() / 1024));
    d.font(ScreenFont::Tiny);
    d.text(0, y, buf);
}
