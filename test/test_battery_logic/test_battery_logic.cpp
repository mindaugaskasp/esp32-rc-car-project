#include <unity.h>
#include "drivers/battery/BatteryLogic.h"

void setUp() {}
void tearDown() {}

// --- Divider scaling (module is 30k/7.5k → ×5) ---

void test_divider_scales_by_module_ratio() {
    // Literal 5x divider (40k/10k), so rewiring the real resistors cannot fail this.
    TEST_ASSERT_EQUAL(7400, batteryMillivoltsFromAdc(1480, 40000, 10000));
    TEST_ASSERT_EQUAL(8400, batteryMillivoltsFromAdc(1680, 40000, 10000));
}

void test_zero_input_reads_zero() {
    TEST_ASSERT_EQUAL(0, batteryMillivoltsFromAdc(0, BATTERY_DIVIDER_TOP_OHMS, BATTERY_DIVIDER_BOTTOM_OHMS));
}

void test_negative_input_clamps_to_zero() {
    TEST_ASSERT_EQUAL(0, batteryMillivoltsFromAdc(-100, BATTERY_DIVIDER_TOP_OHMS, BATTERY_DIVIDER_BOTTOM_OHMS));
}

void test_invalid_divider_reads_zero() {
    TEST_ASSERT_EQUAL(0, batteryMillivoltsFromAdc(1500, BATTERY_DIVIDER_TOP_OHMS, 0));
    TEST_ASSERT_EQUAL(0, batteryMillivoltsFromAdc(1500, -1, BATTERY_DIVIDER_BOTTOM_OHMS));
}

// --- Calibration trim ---

void test_identity_calibration_is_unchanged() {
    TEST_ASSERT_EQUAL(7400, applyBatteryCalibration(7400, 1000));
}

void test_calibration_scales_per_mille() {
    TEST_ASSERT_EQUAL(7770, applyBatteryCalibration(7400, 1050));
    TEST_ASSERT_EQUAL(7030, applyBatteryCalibration(7400, 950));
}

void test_invalid_calibration_reads_zero() {
    TEST_ASSERT_EQUAL(0, applyBatteryCalibration(7400, 0));
    TEST_ASSERT_EQUAL(0, applyBatteryCalibration(-1, 1000));
}

// --- EMA smoothing ---

void test_first_sample_primes_filter() {
    TEST_ASSERT_EQUAL(7400, smoothBatteryMillivolts(-1, 7400, 1, 4));
}

void test_smoothing_moves_fractionally_toward_sample() {
    // NUM/DEN = 1/4 → moves a quarter of the way: 8000 + (7000-8000)/4 = 7750
    TEST_ASSERT_EQUAL(7750, smoothBatteryMillivolts(8000, 7000, 1, 4));
    TEST_ASSERT_EQUAL(8250, smoothBatteryMillivolts(8000, 9000, 1, 4));
}

void test_smoothing_converges_on_steady_input() {
    int smoothed = 8400;
    for (int sample = 0; sample < 100; sample++) {
        smoothed = smoothBatteryMillivolts(smoothed, 7000, BATTERY_SMOOTHING_NUM, BATTERY_SMOOTHING_DEN);
    }
    TEST_ASSERT_EQUAL(7000, smoothed);
}

void test_degenerate_smoothing_config_passes_sample_through() {
    TEST_ASSERT_EQUAL(7000, smoothBatteryMillivolts(8000, 7000, 1, 0));
    TEST_ASSERT_EQUAL(7000, smoothBatteryMillivolts(8000, 7000, 0, 4));
    TEST_ASSERT_EQUAL(7000, smoothBatteryMillivolts(8000, 7000, 4, 4));
}

// --- Low-voltage warning latch ---

void test_warning_trips_below_threshold() {
    TEST_ASSERT_TRUE(updateLowVoltageWarning(false, BATTERY_CAR_LOW_MILLIVOLTS - 1,
                                             BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
}

void test_no_warning_at_or_above_threshold() {
    TEST_ASSERT_FALSE(updateLowVoltageWarning(false, BATTERY_CAR_LOW_MILLIVOLTS,
                                              BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
    TEST_ASSERT_FALSE(updateLowVoltageWarning(false, 8400,
                                              BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
}

void test_warning_holds_through_hysteresis_band() {
    // Recovered above the trip point but not past the hysteresis — stays latched
    TEST_ASSERT_TRUE(updateLowVoltageWarning(true, BATTERY_CAR_LOW_MILLIVOLTS + BATTERY_WARNING_HYSTERESIS_MILLIVOLTS - 1,
                                             BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
}

void test_warning_clears_above_hysteresis_band() {
    TEST_ASSERT_FALSE(updateLowVoltageWarning(true, BATTERY_CAR_LOW_MILLIVOLTS + BATTERY_WARNING_HYSTERESIS_MILLIVOLTS,
                                              BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
}

void test_no_reading_never_warns() {
    TEST_ASSERT_FALSE(updateLowVoltageWarning(false, 0, BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
    TEST_ASSERT_FALSE(updateLowVoltageWarning(true, 0, BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
    TEST_ASSERT_FALSE(updateLowVoltageWarning(true, -100, BATTERY_CAR_LOW_MILLIVOLTS, BATTERY_WARNING_HYSTERESIS_MILLIVOLTS));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_divider_scales_by_module_ratio);
    RUN_TEST(test_zero_input_reads_zero);
    RUN_TEST(test_negative_input_clamps_to_zero);
    RUN_TEST(test_invalid_divider_reads_zero);
    RUN_TEST(test_identity_calibration_is_unchanged);
    RUN_TEST(test_calibration_scales_per_mille);
    RUN_TEST(test_invalid_calibration_reads_zero);
    RUN_TEST(test_first_sample_primes_filter);
    RUN_TEST(test_smoothing_moves_fractionally_toward_sample);
    RUN_TEST(test_smoothing_converges_on_steady_input);
    RUN_TEST(test_degenerate_smoothing_config_passes_sample_through);
    RUN_TEST(test_warning_trips_below_threshold);
    RUN_TEST(test_no_warning_at_or_above_threshold);
    RUN_TEST(test_warning_holds_through_hysteresis_band);
    RUN_TEST(test_warning_clears_above_hysteresis_band);
    RUN_TEST(test_no_reading_never_warns);
    return UNITY_END();
}
