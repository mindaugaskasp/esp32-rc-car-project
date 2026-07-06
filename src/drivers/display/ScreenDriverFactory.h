#pragma once

#include "ScreenDriver.h"

// Returns the single display driver instance (function-local static — no heap).
ScreenDriver* createScreenDriver();
