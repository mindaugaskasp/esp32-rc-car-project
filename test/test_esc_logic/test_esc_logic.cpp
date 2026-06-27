#include <unity.h>
#include "drivers/esc/EscLogic.h"

void setUp() {}
void tearDown() {}

// --- Endpoints ---

void test_full_brake_returns_min() {
    TEST_ASSERT_EQUAL(ESC_MIN_MICROS, computeEscMicros(0));
}

void test_full_throttle_returns_max() {
    TEST_ASSERT_EQUAL(ESC_MAX_MICROS, computeEscMicros(4095));
}

// --- Deadzone ---

void test_center_is_in_deadzone() {
    TEST_ASSERT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(2048));
}

void test_deadzone_lower_edge_is_neutral() {
    // rawY=1901: 1901 > 1900 && 1901 < 2200 -> neutral
    TEST_ASSERT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(1901));
}

void test_deadzone_upper_edge_is_neutral() {
    // rawY=2199: 2199 > 1900 && 2199 < 2200 -> neutral
    TEST_ASSERT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(2199));
}

void test_boundary_1900_not_in_deadzone() {
    // Condition is rawY > 1900, so 1900 itself is NOT in deadzone
    TEST_ASSERT_NOT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(1900));
}

void test_boundary_2200_not_in_deadzone() {
    // Condition is rawY < 2200, so 2200 itself is NOT in deadzone
    TEST_ASSERT_NOT_EQUAL(ESC_NEUTRAL_MICROS, computeEscMicros(2200));
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

void test_known_value_below_deadzone() {
    // map(1000, 0, 4095, 1100, 1900): 1000*800/4095 + 1100 = 195 + 1100 = 1295
    TEST_ASSERT_EQUAL(1295, computeEscMicros(1000));
}

void test_known_value_above_deadzone() {
    // map(3000, 0, 4095, 1100, 1900): 3000*800/4095 + 1100 = 586 + 1100 = 1686
    TEST_ASSERT_EQUAL(1686, computeEscMicros(3000));
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
    RUN_TEST(test_center_is_in_deadzone);
    RUN_TEST(test_deadzone_lower_edge_is_neutral);
    RUN_TEST(test_deadzone_upper_edge_is_neutral);
    RUN_TEST(test_boundary_1900_not_in_deadzone);
    RUN_TEST(test_boundary_2200_not_in_deadzone);
    RUN_TEST(test_negative_input_clamped_to_zero);
    RUN_TEST(test_over_range_input_clamped_to_max);
    RUN_TEST(test_known_value_below_deadzone);
    RUN_TEST(test_known_value_above_deadzone);
    RUN_TEST(test_output_always_within_esc_range);
    return UNITY_END();
}
