#include "Esp32SysInfo.h"
#include <Arduino.h>

uint8_t getRamUsedPercent() {
    uint32_t total = ESP.getHeapSize();
    if (total == 0) return 0;
    uint32_t used = total - ESP.getFreeHeap();
    return (uint8_t)(used * 100UL / total);
}

uint8_t getFlashUsedPercent() {
    uint32_t used  = ESP.getSketchSize();
    uint32_t total = used + ESP.getFreeSketchSpace();
    if (total == 0) return 0;
    return (uint8_t)(used * 100UL / total);
}

uint32_t getFreeHeapBytes() {
    return ESP.getFreeHeap();
}
