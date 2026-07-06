#include <unity.h>
#include "drivers/servo/ServoLogic.h"

// Assertions reference the config symbols directly (SERVO_STEERING_CENTER_RAW,
// STEERING_JOY_MIN/MAX, SERVO_MIN/MAX_MICROS), so they hold regardless of the tuned
// values. The only literal below is the trimmed center:
//   center = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS (1500 + 80 = 1580).

static const int CENTER = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS; // 1580

void setUp() {}
void tearDown() {}

// --- Center reads neutral; deadzone is now removed transmitter-side ---

void test_center_returns_neutral_with_trim() {
    TEST_ASSERT_EQUAL(CENTER, computeServoMicros(SERVO_STEERING_CENTER_RAW));
}

void test_just_right_of_center_exceeds_center() {
    TEST_ASSERT_GREATER_THAN(CENTER, computeServoMicros(SERVO_STEERING_CENTER_RAW + 50));
}

void test_just_left_of_center_below_center() {
    TEST_ASSERT_LESS_THAN(CENTER, computeServoMicros(SERVO_STEERING_CENTER_RAW - 50));
}

// --- Calibrated endpoints ---

void test_full_right_returns_servo_max() {
    // map(3950, 2048, 3950, 1580, 2500): (3950-2048)*920/1902 + 1580 = 920 + 1580 = 2500
    TEST_ASSERT_EQUAL(SERVO_MAX_MICROS, computeServoMicros(STEERING_JOY_MAX));
}

void test_full_left_returns_servo_min() {
    // map(100, 100, 2048, 500, 1580): (100-100)*1080/1948 + 500 = 0 + 500 = 500
    TEST_ASSERT_EQUAL(SERVO_MIN_MICROS, computeServoMicros(STEERING_JOY_MIN));
}

// --- Clamping beyond joystick calibration range ---

void test_raw_above_joystick_max_clamped_to_servo_max() {
    // rawX=4095 maps beyond STEERING_JOY_MAX and gets clamped to SERVO_MAX_MICROS
    TEST_ASSERT_EQUAL(SERVO_MAX_MICROS, computeServoMicros(4095));
}

void test_raw_below_joystick_min_clamped_to_servo_min() {
    // rawX=0 maps below STEERING_JOY_MIN and gets clamped to SERVO_MIN_MICROS
    TEST_ASSERT_EQUAL(SERVO_MIN_MICROS, computeServoMicros(0));
}

void test_negative_input_clamped() {
    TEST_ASSERT_EQUAL(computeServoMicros(0), computeServoMicros(-100));
}

void test_over_range_input_clamped() {
    TEST_ASSERT_EQUAL(computeServoMicros(4095), computeServoMicros(5000));
}

// --- Output always in valid PWM range ---

void test_output_always_within_servo_range() {
    for (int raw = 0; raw <= 4095; raw += 10) {
        int result = computeServoMicros(raw);
        TEST_ASSERT_GREATER_OR_EQUAL(SERVO_MIN_MICROS, result);
        TEST_ASSERT_LESS_OR_EQUAL(SERVO_MAX_MICROS, result);
    }
}

// --- Monotonicity: output must be non-decreasing across the full input range ---

void test_output_monotonically_non_decreasing() {
    int prev = computeServoMicros(0);
    for (int raw = 1; raw <= 4095; raw++) {
        int curr = computeServoMicros(raw);
        TEST_ASSERT_GREATER_OR_EQUAL(prev, curr);
        prev = curr;
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_center_returns_neutral_with_trim);
    RUN_TEST(test_just_right_of_center_exceeds_center);
    RUN_TEST(test_just_left_of_center_below_center);
    RUN_TEST(test_full_right_returns_servo_max);
    RUN_TEST(test_full_left_returns_servo_min);
    RUN_TEST(test_raw_above_joystick_max_clamped_to_servo_max);
    RUN_TEST(test_raw_below_joystick_min_clamped_to_servo_min);
    RUN_TEST(test_negative_input_clamped);
    RUN_TEST(test_over_range_input_clamped);
    RUN_TEST(test_output_always_within_servo_range);
    RUN_TEST(test_output_monotonically_non_decreasing);
    return UNITY_END();
}
