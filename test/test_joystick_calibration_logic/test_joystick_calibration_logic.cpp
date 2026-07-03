#include <unity.h>
#include "ui/calibration/JoystickCalibrationLogic.h"

void setUp() {}
void tearDown() {}

// --- Center raw (integer mean of resting samples) ---

void test_center_is_integer_mean() {
    // sum=4100 over 2 samples -> 2050
    TEST_ASSERT_EQUAL(2050, computeCenterRaw(4100, 2));
}

void test_center_truncates_toward_zero() {
    // 5 samples summing 10237 -> 2047 (10237/5 = 2047.4)
    TEST_ASSERT_EQUAL(2047, computeCenterRaw(10237, 5));
}

void test_center_no_samples_returns_midpoint() {
    TEST_ASSERT_EQUAL(CALIBRATION_ADC_MIDPOINT, computeCenterRaw(0, 0));
    TEST_ASSERT_EQUAL(CALIBRATION_ADC_MIDPOINT, computeCenterRaw(12345, -3));
}

// --- Deadzone suggestion (scaled resting jitter, floored) ---

void test_deadzone_scales_with_jitter() {
    // center 2048, rest 2038..2060 -> max deviation max(10,12)=12 -> *2 = 24
    TEST_ASSERT_EQUAL(24, suggestDeadzone(2048, 2038, 2060));
}

void test_deadzone_uses_larger_side() {
    // low deviation 50, high deviation 5 -> 50*2 = 100
    TEST_ASSERT_EQUAL(100, suggestDeadzone(2048, 1998, 2053));
}

void test_deadzone_floored_when_quiet() {
    // tiny jitter (2 units) -> 4, below floor -> CALIBRATION_DEADZONE_MIN
    TEST_ASSERT_EQUAL(CALIBRATION_DEADZONE_MIN, suggestDeadzone(2048, 2046, 2049));
}

void test_deadzone_zero_jitter_is_floor() {
    TEST_ASSERT_EQUAL(CALIBRATION_DEADZONE_MIN, suggestDeadzone(2048, 2048, 2048));
}

void test_deadzone_never_negative_on_inverted_bounds() {
    // center outside the rest range (degenerate) -> deviation clamped to 0 -> floor
    TEST_ASSERT_EQUAL(CALIBRATION_DEADZONE_MIN, suggestDeadzone(2048, 2060, 2040));
}

void test_deadzone_outlier_is_capped() {
    // Regression: a rail-region rest reading (e.g. 4095) used to yield ~4458.
    // Deviation is clamped to CALIBRATION_MAX_REST_DEVIATION before scaling.
    int expected = CALIBRATION_MAX_REST_DEVIATION * CALIBRATION_DEADZONE_SAFETY_MARGIN;
    TEST_ASSERT_EQUAL(expected, suggestDeadzone(1866, 1866, 4095));
    // Never exceeds the ADC full-scale range.
    TEST_ASSERT_LESS_THAN(4096, suggestDeadzone(1866, 0, 4095));
}

// --- Rest-sample plausibility gate ---

void test_rest_sample_near_center_is_plausible() {
    TEST_ASSERT_TRUE(isPlausibleRestSample(CALIBRATION_ADC_MIDPOINT));
    TEST_ASSERT_TRUE(isPlausibleRestSample(1866)); // real X center seen in the field
    TEST_ASSERT_TRUE(isPlausibleRestSample(2252)); // real Y center seen in the field
}

void test_rail_region_sample_is_rejected() {
    TEST_ASSERT_FALSE(isPlausibleRestSample(0));
    TEST_ASSERT_FALSE(isPlausibleRestSample(4095));
}

void test_rest_sample_gate_boundary() {
    TEST_ASSERT_TRUE(isPlausibleRestSample(CALIBRATION_ADC_MIDPOINT + CALIBRATION_REST_MAX_OFFSET));
    TEST_ASSERT_FALSE(isPlausibleRestSample(CALIBRATION_ADC_MIDPOINT + CALIBRATION_REST_MAX_OFFSET + 1));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_center_is_integer_mean);
    RUN_TEST(test_center_truncates_toward_zero);
    RUN_TEST(test_center_no_samples_returns_midpoint);
    RUN_TEST(test_deadzone_scales_with_jitter);
    RUN_TEST(test_deadzone_uses_larger_side);
    RUN_TEST(test_deadzone_floored_when_quiet);
    RUN_TEST(test_deadzone_zero_jitter_is_floor);
    RUN_TEST(test_deadzone_never_negative_on_inverted_bounds);
    RUN_TEST(test_deadzone_outlier_is_capped);
    RUN_TEST(test_rest_sample_near_center_is_plausible);
    RUN_TEST(test_rail_region_sample_is_rejected);
    RUN_TEST(test_rest_sample_gate_boundary);
    return UNITY_END();
}
