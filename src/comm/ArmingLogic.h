#pragma once
#include "config/ControlConfig.h"

// Failsafe arming for the receiver's ESC: after boot and after every packet-loss
// timeout, the throttle must be seen at neutral for a short run of commands before
// propulsion is allowed. Counting only near-neutral frames (not just any valid
// frame) means a link that drops while the driver holds throttle cannot slam the
// motor from neutral straight back to that throttle on reconnect — the stick has
// to come home first. Standard RC failsafe behavior.

// Number of consecutive near-neutral throttle commands required to arm.
// Keep small — the transmitter's idle heartbeat sends neutral frames every 300ms,
// so arming costs well under a second once the stick is centered.
static const int ARM_COMMAND_THRESHOLD = 2;

// How far (in conditioned ADC units) a throttle command may sit from
// THROTTLE_CENTER_RAW and still count as neutral for arming. The transmitter
// snaps in-deadzone sticks exactly to center, so this only needs to tolerate a
// barely-deflected stick; 100 units ≈ 20µs of ESC pulse — no meaningful motion.
static const int ARM_NEUTRAL_THROTTLE_BAND = 100;

inline bool isNeutralThrottleCommand(int throttleCommand) {
    int deflection = throttleCommand - THROTTLE_CENTER_RAW;
    if (deflection < 0) deflection = -deflection;
    return deflection <= ARM_NEUTRAL_THROTTLE_BAND;
}

inline bool isArmed(int armingCount) {
    return armingCount >= ARM_COMMAND_THRESHOLD;
}

// Advances the arming counter for one received command. Once armed, stays armed
// (the counter is reset externally on packet-loss timeout); while disarmed, a
// non-neutral throttle command restarts the count from zero.
inline int nextArmingCount(int armingCount, int throttleCommand) {
    if (isArmed(armingCount)) return armingCount;
    return isNeutralThrottleCommand(throttleCommand) ? armingCount + 1 : 0;
}
