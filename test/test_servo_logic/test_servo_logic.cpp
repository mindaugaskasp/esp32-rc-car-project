#include <unity.h>
#include "drivers/servo/ServoLogic.h"

// Config values (from ControlConfig.h):
//   SERVO_NEUTRAL_MICROS=1500, SERVO_CENTER_TRIM_MICROS=80  -> center=1580
//   SERVO_MIN_MICROS=500, SERVO_MAX_MICROS=2500
//   JOY_DEADZONE_X_X=150, SERVO_JOYSTICK_CENTER_RAW=2048
//   JOYSTICK_X_MIN=100, JOYSTICK_X_MAX=3950

static const int CENTER = SERVO_NEUTRAL_MICROS + SERVO_CENTER_TRIM_MICROS; // 1580

void setUp() {}
void tearDown() {}

// --- Center / deadzone ---

void test_center_returns_neutral_with_trim() {
    TEST_ASSERT_EQUAL(CENTER, computeServoMicros(SERVO_JOYSTICK_CENTER_RAW));
}

void test_deadzone_left_boundary_still_neutral() {
    // abs(2048 - 75 - 2048) = 75, not > 75 -> still center
    TEST_ASSERT_EQUAL(CENTER, computeServoMicros(SERVO_JOYSTICK_CENTER_RAW - JOY_DEADZONE_X));
}

void test_deadzone_right_boundary_still_neutral() {
    TEST_ASSERT_EQUAL(CENTER, computeServoMicros(SERVO_JOYSTICK_CENTER_RAW + JOY_DEADZONE_X));
}

void test_just_outside_deadzone_right_exceeds_center() {
    TEST_ASSERT_GREATER_THAN(CENTER, computeServoMicros(SERVO_JOYSTICK_CENTER_RAW + JOY_DEADZONE_X + 1));
}

void test_just_outside_deadzone_left_below_center() {
    TEST_ASSERT_LESS_THAN(CENTER, computeServoMicros(SERVO_JOYSTICK_CENTER_RAW - JOY_DEADZONE_X - 1));
}

// --- Calibrated endpoints ---

void test_full_right_returns_servo_max() {
    // map(3950, 2048, 3950, 1580, 2500): (3950-2048)*920/1902 + 1580 = 920 + 1580 = 2500
    TEST_ASSERT_EQUAL(SERVO_MAX_MICROS, computeServoMicros(JOYSTICK_X_MAX));
}

void test_full_left_returns_servo_min() {
    // map(100, 100, 2048, 500, 1580): (100-100)*1080/1948 + 500 = 0 + 500 = 500
    TEST_ASSERT_EQUAL(SERVO_MIN_MICROS, computeServoMicros(JOYSTICK_X_MIN));
}

// --- Clamping beyond joystick calibration range ---

void test_raw_above_joystick_max_clamped_to_servo_max() {
    // rawX=4095 maps beyond JOYSTICK_X_MAX and gets clamped to SERVO_MAX_MICROS
    TEST_ASSERT_EQUAL(SERVO_MAX_MICROS, computeServoMicros(4095));
}

void test_raw_below_joystick_min_clamped_to_servo_min() {
    // rawX=0 maps below JOYSTICK_X_MIN and gets clamped to SERVO_MIN_MICROS
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
    RUN_TEST(test_deadzone_left_boundary_still_neutral);
    RUN_TEST(test_deadzone_right_boundary_still_neutral);
    RUN_TEST(test_just_outside_deadzone_right_exceeds_center);
    RUN_TEST(test_just_outside_deadzone_left_below_center);
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
