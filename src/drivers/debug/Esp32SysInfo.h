#pragma once

#include <stdint.h>

// Returns heap RAM used as a percentage (0-100).
uint8_t getRamUsedPercent();

// Returns sketch-partition flash used as a percentage (0-100).
uint8_t getFlashUsedPercent();

// Returns free heap in bytes.
uint32_t getFreeHeapBytes();
