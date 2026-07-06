#pragma once

// Physical DEBUG-mode toggle switch on the transmitter (active LOW, INPUT_PULLUP,
// wired to GND — see DEBUG_MODE_SWITCH_PIN). While ON it reveals the Debug Info
// menu entry, the dashboard DEBUG MODE badge, and the rotating link-stat readout.
// Read live each loop so flipping the switch takes effect without a restart.
// Transmitter-only: the headless receiver has no such switch.
void initDebugModeSwitch();
bool isDebugModeActive();
