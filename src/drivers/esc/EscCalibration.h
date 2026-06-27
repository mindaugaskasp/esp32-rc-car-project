#pragma once

#include <ESP32Servo.h>

// ─── Parameter enums (match tech sheet table rows 1-17) ──────────────────────

enum class EscOperationMode : uint8_t {
    PositiveRotationBandBrake       = 1,
    DirectPositiveNegativeInversion = 2,
    PositiveBeltProportionalBrake   = 3,
    PositiveAndReverseBelt          = 4,
};

enum class EscMotorDirection : uint8_t {
    PositiveRotation = 1,
    Reversal         = 2,
};

enum class EscStartMode : uint8_t {
    Level1 = 1,
    Level2 = 2,
    Level3 = 3,
};

// Rows 4-9 share a common 10-level percentage scale; use raw uint8_t 1-10.
// Helper constants for readability:
static constexpr uint8_t ESC_LEVEL_1  = 1;
static constexpr uint8_t ESC_LEVEL_2  = 2;
static constexpr uint8_t ESC_LEVEL_3  = 3;
static constexpr uint8_t ESC_LEVEL_4  = 4;
static constexpr uint8_t ESC_LEVEL_5  = 5;
static constexpr uint8_t ESC_LEVEL_6  = 6;
static constexpr uint8_t ESC_LEVEL_7  = 7;
static constexpr uint8_t ESC_LEVEL_8  = 8;
static constexpr uint8_t ESC_LEVEL_9  = 9;
static constexpr uint8_t ESC_LEVEL_10 = 10;

enum class EscBrakeFrequency : uint8_t {
    Hz16000 = 1,
    Hz8000  = 2,
    Hz4000  = 3,
    Hz2000  = 4,
    Hz500   = 5,
    Hz250   = 6,
    Hz125   = 7,
};

enum class EscLithiumCells : uint8_t {
    AutoRecognition = 1,
    S2              = 2,
    S3              = 3,
    S4              = 4,
    S5              = 5,
    S6              = 6,
};

// Row 13: Low Voltage Protection Threshold (per cell, 2.6V–3.7V in 0.1V steps)
enum class EscLowVoltageThreshold : uint8_t {
    V2_6  = 1,
    V2_7  = 2,
    V2_8  = 3,
    V2_9  = 4,
    V3_0  = 5,
    V3_1  = 6,
    V3_2  = 7,
    V3_3  = 8,
    V3_4  = 9,
    V3_5  = 10,
    V3_6  = 11,
    V3_7  = 12,
};

enum class EscLowVoltageProtection : uint8_t {
    NoProtection = 1,
    Protect      = 2,
};

// Row 15: Reverse max throttle stroke (0.9ms–1.2ms, 12 steps)
enum class EscReverseThrottleStroke : uint8_t {
    Ms0_900  = 1,
    Ms0_933  = 2,
    Ms0_967  = 3,
    Ms1_000  = 4,
    Ms1_033  = 5,
    Ms1_067  = 6,
    Ms1_100  = 7,
    Ms1_133  = 8,
    Ms1_167  = 9,
    Ms1_200  = 10,
};

// Row 16: Forward max throttle stroke (1.8ms–2.1ms, 10 steps)
enum class EscForwardThrottleStroke : uint8_t {
    Ms1_800  = 1,
    Ms1_833  = 2,
    Ms1_867  = 3,
    Ms1_900  = 4,
    Ms1_933  = 5,
    Ms1_967  = 6,
    Ms2_000  = 7,
    Ms2_033  = 8,
    Ms2_067  = 9,
    Ms2_100  = 10,
};

enum class EscSynchronizedRectification : uint8_t {
    Close = 1,
    Open  = 2,
};

// ─── Config struct (defaults = position 1 for each row) ──────────────────────

struct EscConfig {
    EscOperationMode          operationMode        = EscOperationMode::PositiveRotationBandBrake;
    EscMotorDirection         motorDirection        = EscMotorDirection::PositiveRotation;
    EscStartMode              startMode             = EscStartMode::Level1;
    uint8_t                   minForwardStrength    = ESC_LEVEL_1;  // 5%
    uint8_t                   minBackingStrength    = ESC_LEVEL_1;  // 6%
    uint8_t                   maxBackingStrength    = ESC_LEVEL_1;  // 23%
    uint8_t                   initialBrakingStrength = ESC_LEVEL_1; // 0%
    uint8_t                   maxBrakingStrength    = ESC_LEVEL_1;  // 0%
    uint8_t                   brakingForce          = ESC_LEVEL_1;  // 0%
    uint8_t                   neutralPointRange     = ESC_LEVEL_1;  // 2%
    EscBrakeFrequency         brakeFrequency        = EscBrakeFrequency::Hz16000;
    EscLithiumCells           lithiumCells          = EscLithiumCells::AutoRecognition;
    EscLowVoltageThreshold    lowVoltageThreshold   = EscLowVoltageThreshold::V2_6;
    EscLowVoltageProtection   lowVoltageProtection  = EscLowVoltageProtection::NoProtection;
    EscReverseThrottleStroke  reverseThrottleStroke = EscReverseThrottleStroke::Ms0_900;
    EscForwardThrottleStroke  forwardThrottleStroke = EscForwardThrottleStroke::Ms1_800;
    EscSynchronizedRectification syncRectification  = EscSynchronizedRectification::Close;
};

// ─── Calibration class ────────────────────────────────────────────────────────

class EscCalibration {
public:
    // Run the standard throttle-range calibration sequence.
    // Call this BEFORE powering the ESC, then power the ESC while this runs.
    // Sequence: full throttle → wait for beep → full brake → wait for beep → neutral.
    // minMicros/maxMicros must match the ESC_MIN/MAX values used by EscDriver.
    void calibrateThrottleRange(Servo& esc, int minMicros, int neutralMicros, int maxMicros);

    // Program all 17 ESC parameters via the signal wire (replicates the programming card).
    // Call this BEFORE powering the ESC, then power the ESC at the start of the sequence.
    //
    // Protocol (same signal wire as normal throttle):
    //   1. Hold full throttle during ESC power-on → ESC enters programming mode.
    //   2. Drop to full brake → programming mode confirmed.
    //   3. For each parameter row: N brief throttle pulses advance from value 1 to target N,
    //      then a long brake hold confirms the value and advances to the next row.
    //   4. Hold full throttle at the end to save all values and exit.
    //
    // ASSUMPTIONS:
    //   - The ESC resets each parameter to position 1 on entering programming mode.
    //   - Timing constants below may need empirical adjustment for your specific ESC.
    //   - Previously stored non-default values may cause row-count drift.
    void programParameters(Servo& esc, int minMicros, int neutralMicros, int maxMicros, const EscConfig& config);
};

extern EscCalibration escCalibration;
