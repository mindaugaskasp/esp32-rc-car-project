#include "EscCalibration.h"
#include <Arduino.h>

EscCalibration escCalibration;

void EscCalibration::calibrateThrottleRange(Servo& esc, int minMicros, int neutralMicros, int maxMicros) {
    // Step 1: Hold full throttle so the ESC can learn the maximum endpoint.
    // Power on the ESC now — it will emit the startup "Beep-Beep" tone.
    esc.writeMicroseconds(maxMicros);
    delay(3000);

    // Step 2: Move to full brake so the ESC learns the minimum endpoint.
    // The ESC emits a confirmation tone when the neutral position is recognised.
    esc.writeMicroseconds(minMicros);
    delay(2000);

    // Step 3: Return to neutral — ESC is now calibrated and workable.
    esc.writeMicroseconds(neutralMicros);
}

// ─── Stick-programming timing constants (milliseconds) ───────────────────────
// These replicate the programming card protocol over the signal wire.
// Adjust if the ESC does not respond correctly — start by doubling CONFIRM_MS.
static const int ESC_PROG_ENTRY_THROTTLE_MS = 5000; // Hold max during boot for prog-mode entry
static const int ESC_PROG_ENTRY_BRAKE_MS    = 3000; // Hold min to confirm prog-mode entry
static const int ESC_PROG_ADVANCE_PULSE_MS  =  500; // Brief max pulse to advance one value step
static const int ESC_PROG_ADVANCE_SETTLE_MS =  300; // Min hold between advance pulses
static const int ESC_PROG_CONFIRM_MS        = 1500; // Hold min to confirm value + move to next row
static const int ESC_PROG_EXIT_MS           = 3000; // Hold max to save all and exit

void EscCalibration::programParameters(Servo& esc, int minMicros, int neutralMicros, int maxMicros, const EscConfig& config) {
    // Flatten config into an ordered array matching datasheet rows 1-17.
    const uint8_t values[17] = {
        (uint8_t)config.operationMode,
        (uint8_t)config.motorDirection,
        (uint8_t)config.startMode,
        config.minForwardStrength,
        config.minBackingStrength,
        config.maxBackingStrength,
        config.initialBrakingStrength,
        config.maxBrakingStrength,
        config.brakingForce,
        config.neutralPointRange,
        (uint8_t)config.brakeFrequency,
        (uint8_t)config.lithiumCells,
        (uint8_t)config.lowVoltageThreshold,
        (uint8_t)config.lowVoltageProtection,
        (uint8_t)config.reverseThrottleStroke,
        (uint8_t)config.forwardThrottleStroke,
        (uint8_t)config.syncRectification,
    };

    // Step 1: Hold full throttle — power on the ESC now.
    // The ESC will emit startup beeps then enter programming mode.
    esc.writeMicroseconds(maxMicros);
    delay(ESC_PROG_ENTRY_THROTTLE_MS);

    // Step 2: Drop to full brake to confirm programming mode entry.
    esc.writeMicroseconds(minMicros);
    delay(ESC_PROG_ENTRY_BRAKE_MS);

    // Step 3: Program each row in order.
    // Assumption: each row starts at value 1 when entering programming mode.
    // Advance from value 1 to the target with brief throttle pulses, then confirm.
    for (int row = 0; row < 17; row++) {
        const uint8_t target = values[row];

        for (uint8_t step = 1; step < target; step++) {
            esc.writeMicroseconds(maxMicros);
            delay(ESC_PROG_ADVANCE_PULSE_MS);
            esc.writeMicroseconds(minMicros);
            delay(ESC_PROG_ADVANCE_SETTLE_MS);
        }

        // Long brake hold = confirm current value and advance to next row.
        esc.writeMicroseconds(minMicros);
        delay(ESC_PROG_CONFIRM_MS);
    }

    // Step 4: Hold full throttle to save all confirmed values and exit.
    esc.writeMicroseconds(maxMicros);
    delay(ESC_PROG_EXIT_MS);

    esc.writeMicroseconds(neutralMicros);
}
