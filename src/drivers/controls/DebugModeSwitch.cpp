#include "DebugModeSwitch.h"
#include "drivers/controls/Controls.h"
#include "config/controller/Esp32Pins.h"

void initDebugModeSwitch() {
    initButton(DEBUG_MODE_SWITCH_PIN);
}

bool isDebugModeActive() {
    return readButton(DEBUG_MODE_SWITCH_PIN);
}
