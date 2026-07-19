#pragma once

// Battery voltage sensing on both boards via the common "Voltage Sensor"
// divider module (marked VCC<25V): battery + / − on the screw terminals, module
// S pin → BATTERY_SENSE_PIN, module − pin → ESP32 GND (shared ground required).
// Kept as #define for native-test / Arduino parity — see CLAUDE.md.

// The module is a fixed 30k/7.5k divider: Vbat = Vpin × (TOP + BOTTOM) / BOTTOM
// = Vpin × 5. A full 2S car pack (8.4V) puts 1.68V on the pin, a full 1S remote
// cell (4.2V) puts 0.84V — both inside the ADC's calibrated range at the
// default 11dB attenuation (accurate up to ~2.5V).
#define BATTERY_DIVIDER_TOP_OHMS 30000L
#define BATTERY_DIVIDER_BOTTOM_OHMS 7500L

// Per-board trim (per-mille) applied after the divider math: measure the pack
// with a multimeter and scale until the displayed voltage matches (1000 = no
// correction). Each board carries its own divider module, so each build gets
// its own value — they absorb that module's resistor tolerance and any residual
// ADC calibration error. (Native tests take the receiver branch; the value is
// irrelevant to them.)
#ifdef IS_TRANSMITTER
#define BATTERY_CALIBRATION_PER_MILLE 1000
#else
#define BATTERY_CALIBRATION_PER_MILLE 1000
#endif

// Sampling cadence and noise handling. Each sample averages a burst of ADC
// reads (motor PWM couples noise onto the rail), then an EMA smooths across
// samples: NUM/DEN = weight of the newest sample.
#define BATTERY_SAMPLE_INTERVAL_MS 500
#define BATTERY_ADC_BURST_READS 8
#define BATTERY_SMOOTHING_NUM 1
#define BATTERY_SMOOTHING_DEN 4

// Low-voltage warning thresholds (dashboard "LOW BATT" alert, see
// updateLowVoltageWarning in BatteryLogic.h). Car: 2S LiPo, 3.5V/cell ≈ 20%
// charge left. Remote: 1S Li-ion. Once tripped the warning clears only
// HYSTERESIS above the threshold, so a pack sagging under throttle load and
// rebounding at idle doesn't flicker the alert.
#define BATTERY_CAR_LOW_MILLIVOLTS 7000
#define BATTERY_REMOTE_LOW_MILLIVOLTS 3500
#define BATTERY_WARNING_HYSTERESIS_MILLIVOLTS 150
