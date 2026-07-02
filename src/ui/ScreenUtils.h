#pragma once

#include "drivers/display/ScreenDriver.h"
#include <stdint.h>

// Shared rendering helpers for calibration screens.
// All functions clear the buffer, draw, and flush.

void drawCalibStep(ScreenDriver& d, const char* title, uint8_t step,
                   uint8_t totalSteps, const char* instruction, int barValue);

void drawCalibResult(ScreenDriver& d, const char* title,
                     const char* line1, const char* line2, const char* line3);

// Calibration menu with live RAM/Flash stats in the header and nav hint footer.
void drawCalibMenu(ScreenDriver& d, const char* above,
                   const char* selected, const char* below);

// Draws a one-line "RAM:XX%  FSH:XX%  YYkB" footer at baseline y using Tiny font.
// Does NOT clear or flush — call inside an existing clear/flush pair.
void drawSysInfoFooter(ScreenDriver& d, int y);
