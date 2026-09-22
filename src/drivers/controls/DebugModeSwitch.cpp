#include "DebugModeSwitch.h"
#include "drivers/controls/Controls.h"
#include "config/controller/Esp32Pins.h"
#include "config/DebugConfig.h"

void initDebugModeSwitch() {
    initButton(DEBUG_MODE_SWITCH_PIN);
}

bool isDebugModeActive() {
    return DEBUG_MODE_FORCE_ON || readButton(DEBUG_MODE_SWITCH_PIN);
}
