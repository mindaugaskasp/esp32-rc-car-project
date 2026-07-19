#include "BatteryMonitorDriver.h"
#include "BatteryLogic.h"
#include <config/controller/Esp32Pins.h>
#include <config/ControlConfig.h>
#include <Arduino.h>

static unsigned long lastSampleTimeMs = 0;
static int smoothedMillivolts = -1; // negative = filter not primed (see BatteryLogic.h)

static int readBatteryMillivolts() {
    // Burst-average: motor PWM couples noise onto the battery rail, and
    // analogReadMilliVolts already applies the factory ADC calibration.
    long burstSumMillivolts = 0;
    for (int readIndex = 0; readIndex < BATTERY_ADC_BURST_READS; readIndex++) {
        burstSumMillivolts += analogReadMilliVolts(BATTERY_SENSE_PIN);
    }
    int adcMillivolts = static_cast<int>(burstSumMillivolts / BATTERY_ADC_BURST_READS);
    int packMillivolts = batteryMillivoltsFromAdc(adcMillivolts,
                                                  BATTERY_DIVIDER_TOP_OHMS, BATTERY_DIVIDER_BOTTOM_OHMS);
    return applyBatteryCalibration(packMillivolts, BATTERY_CALIBRATION_PER_MILLE);
}

void initBatteryMonitor() {
    // Prime the filter so the very first telemetry frame carries a real voltage
    // instead of 0.0 for the first sample interval.
    smoothedMillivolts = readBatteryMillivolts();
    lastSampleTimeMs = millis();
}

void updateBatteryMonitor() {
    unsigned long now = millis();
    if (now - lastSampleTimeMs < BATTERY_SAMPLE_INTERVAL_MS) return;
    lastSampleTimeMs = now;
    smoothedMillivolts = smoothBatteryMillivolts(smoothedMillivolts, readBatteryMillivolts(),
                                                 BATTERY_SMOOTHING_NUM, BATTERY_SMOOTHING_DEN);
}

float getBatteryVoltage() {
    if (smoothedMillivolts < 0) return 0.0f;
    return static_cast<float>(smoothedMillivolts) / 1000.0f;
}
