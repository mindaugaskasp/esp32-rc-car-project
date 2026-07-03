#include <unity.h>
#include "drivers/esc/EscLogic.h"

void setUp() {}
void tearDown() {}

// --- Endpoints ---

void test_full_brake_returns_min() {
    TEST_ASSERT_EQUAL(ESC_MIN_MICROS, computeEscMicros(THROTTLE_JOY_MIN));
}

void test_full_throttle_returns_max() {
    TEST_ASSERT_EQUAL(ESC_MAX_MICROS, computeEscMicros(THROTTLE_JOY_MAX));
}

// --- Deadzone (symmetric around the measured throttle center) ---

void test_center_is_neutral() {
    TEST_ASSERT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(THROTTLE_CENTER_RAW));
}

// Regression: the joystick rests near THROTTLE_CENTER_RAW, not the nominal 2048.
// Every value in the observed resting-jitter window must map to neutral so the
// motor stays stopped at rest (previously ~2225 fell outside the old band and the
// motor idled forward).
void test_resting_jitter_stays_neutral() {
    for (int rawY = 2222; rawY <= 2231; rawY++) {
        TEST_ASSERT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(rawY));
    }
}

void test_deadzone_edges_are_neutral() {
    // diff == JOY_DEADZONE_Y is NOT outside the deadzone (condition is diff > deadzone)
    TEST_ASSERT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(THROTTLE_CENTER_RAW + JOY_DEADZONE_Y));
    TEST_ASSERT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(THROTTLE_CENTER_RAW - JOY_DEADZONE_Y));
}

void test_just_outside_deadzone_is_not_neutral() {
    TEST_ASSERT_NOT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(THROTTLE_CENTER_RAW + JOY_DEADZONE_Y + 1));
    TEST_ASSERT_NOT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(THROTTLE_CENTER_RAW - JOY_DEADZONE_Y - 1));
}

// --- Bidirectional mapping: below-center is reverse, above-center is forward ---

void test_below_center_is_reverse() {
    TEST_ASSERT_LESS_THAN(ESC_NEUTRAL_MICROS, computeEscMicros(THROTTLE_CENTER_RAW - JOY_DEADZONE_Y - 1));
    TEST_ASSERT_LESS_THAN(ESC_NEUTRAL_MICROS, computeEscMicros(500));
}

void test_above_center_is_forward() {
    TEST_ASSERT_GREATER_THAN(ESC_NEUTRAL_MICROS, computeEscMicros(THROTTLE_CENTER_RAW + JOY_DEADZONE_Y + 1));
    TEST_ASSERT_GREATER_THAN(ESC_NEUTRAL_MICROS, computeEscMicros(3500));
}

// --- Clamping ---

void test_negative_input_clamped_to_zero() {
    TEST_ASSERT_EQUAL(computeEscMicros(0), computeEscMicros(-1));
    TEST_ASSERT_EQUAL(computeEscMicros(0), computeEscMicros(-500));
}

void test_over_range_input_clamped_to_max() {
    TEST_ASSERT_EQUAL(computeEscMicros(4095), computeEscMicros(4096));
    TEST_ASSERT_EQUAL(computeEscMicros(4095), computeEscMicros(9999));
}

// --- Specific mapped values (integer arithmetic matches Arduino map()) ---
// Expected values are derived from the config constants, not hardcoded, so they
// stay correct when THROTTLE_CENTER_RAW is recalibrated. They still pin the mapping
// to the exact Arduino map() integer arithmetic on each side of center.

static int expectedBelowCenter(int rawY) {
    return (int)((long)(rawY - THROTTLE_JOY_MIN) * (ESC_NEUTRAL_MICROS - ESC_MIN_MICROS)
           / (THROTTLE_CENTER_RAW - THROTTLE_JOY_MIN)) + ESC_MIN_MICROS;
}

static int expectedAboveCenter(int rawY) {
    return (int)((long)(rawY - THROTTLE_CENTER_RAW) * (ESC_MAX_MICROS - ESC_NEUTRAL_MICROS)
           / (THROTTLE_JOY_MAX - THROTTLE_CENTER_RAW)) + ESC_NEUTRAL_MICROS;
}

void test_known_value_below_center() {
    TEST_ASSERT_EQUAL(expectedBelowCenter(1000), computeEscMicros(1000));
}

void test_known_value_above_center() {
    TEST_ASSERT_EQUAL(expectedAboveCenter(3000), computeEscMicros(3000));
}

// --- Output always in valid PWM range ---

void test_output_always_within_esc_range() {
    for (int raw = 0; raw <= 4095; raw += 10) {
        int micros = computeEscMicros(raw);
        TEST_ASSERT_GREATER_OR_EQUAL(ESC_MIN_MICROS, micros);
        TEST_ASSERT_LESS_OR_EQUAL(ESC_MAX_MICROS, micros);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_full_brake_returns_min);
    RUN_TEST(test_full_throttle_returns_max);
    RUN_TEST(test_center_is_neutral);
    RUN_TEST(test_resting_jitter_stays_neutral);
    RUN_TEST(test_deadzone_edges_are_neutral);
    RUN_TEST(test_just_outside_deadzone_is_not_neutral);
    RUN_TEST(test_below_center_is_reverse);
    RUN_TEST(test_above_center_is_forward);
    RUN_TEST(test_negative_input_clamped_to_zero);
    RUN_TEST(test_over_range_input_clamped_to_max);
    RUN_TEST(test_known_value_below_center);
    RUN_TEST(test_known_value_above_center);
    RUN_TEST(test_output_always_within_esc_range);
    return UNITY_END();
}
