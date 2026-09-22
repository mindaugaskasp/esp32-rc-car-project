#pragma once

#include "drivers/display/ScreenDriver.h"
#include <stdint.h>

// Shared rendering helpers for the transmitter's screens.
// The drawCalibration* functions clear the buffer, draw, and flush.

// Standard screen header: title in Medium font over a full-width rule. Does NOT
// clear or flush, and leaves the font at Medium — call inside an existing
// clear/flush pair and set the body font afterwards.
void drawScreenHeader(ScreenDriver& driver, const char* title);

void drawCalibrationStep(ScreenDriver& driver, const char* title, uint8_t step,
                   uint8_t totalSteps, const char* instruction, int barValue);

void drawCalibrationResult(ScreenDriver& driver, const char* title,
                     const char* line1, const char* line2, const char* line3);

// Calibration menu with live RAM/Flash stats in the header and nav hint footer.
void drawCalibrationMenu(ScreenDriver& driver, const char* above,
                   const char* selected, const char* below);

// Draws a one-line "RAM:XX%  FSH:XX%  YYkB" footer at baseline y using Tiny font.
// Does NOT clear or flush — call inside an existing clear/flush pair.
void drawSysInfoFooter(ScreenDriver& driver, int y);
