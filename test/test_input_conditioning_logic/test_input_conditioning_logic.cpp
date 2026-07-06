#include <unity.h>
#include "drivers/controls/InputConditioningLogic.h"

// Config-agnostic fixture: a symmetric axis so both sides have span 2000.
static const int CENTER = 2000;
static const int MIN_RAW = 0;
static const int MAX_RAW = 4000;
static const int DEADZONE = 100;
static const int SPAN = 2000;

void setUp() {}
void tearDown() {}

static int linearAxis(int raw) {
    return conditionAxis(raw, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 0, RESPONSE_FULL_SCALE);
}

// --- Deadzone ---

void test_within_deadzone_returns_center() {
    TEST_ASSERT_EQUAL(CENTER, linearAxis(CENTER + 50));
    TEST_ASSERT_EQUAL(CENTER, linearAxis(CENTER - 50));
}

void test_deadzone_edge_returns_center() {
    // magnitude == deadzone is NOT outside (condition is magnitude > deadzone)
    TEST_ASSERT_EQUAL(CENTER, linearAxis(CENTER + DEADZONE));
    TEST_ASSERT_EQUAL(CENTER, linearAxis(CENTER - DEADZONE));
}

void test_just_outside_deadzone_not_center() {
    TEST_ASSERT_NOT_EQUAL(CENTER, linearAxis(CENTER + DEADZONE + 1));
    TEST_ASSERT_NOT_EQUAL(CENTER, linearAxis(CENTER - DEADZONE - 1));
}

// The resting-jitter case that used to live in test_esc_logic: a released stick
// hovering inside the deadzone conditions to exactly center, so the receiver's
// linear map reads neutral and the motor stays stopped.
void test_resting_jitter_conditions_to_center() {
    for (int raw = CENTER - DEADZONE; raw <= CENTER + DEADZONE; raw++) {
        TEST_ASSERT_EQUAL(CENTER, linearAxis(raw));
    }
}

// --- Identity: expo=0, rate=full reproduces the raw reading outside the deadzone ---

void test_linear_is_identity_outside_deadzone() {
    TEST_ASSERT_EQUAL(3000, linearAxis(3000));
    TEST_ASSERT_EQUAL(1000, linearAxis(1000));
    TEST_ASSERT_EQUAL(MAX_RAW, linearAxis(MAX_RAW));
    TEST_ASSERT_EQUAL(MIN_RAW, linearAxis(MIN_RAW));
}

void test_shape_deflection_identity_fast_path() {
    TEST_ASSERT_EQUAL(750, shapeDeflection(750, SPAN, 0, RESPONSE_FULL_SCALE));
}

// --- Expo softens near center but preserves the endpoints ---

void test_expo_softens_near_center() {
    int linearDeflection = linearAxis(CENTER + 200) - CENTER;
    int expoDeflection = conditionAxis(CENTER + 200, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 500, RESPONSE_FULL_SCALE) - CENTER;
    TEST_ASSERT_LESS_THAN(linearDeflection, expoDeflection);
    TEST_ASSERT_GREATER_THAN(0, expoDeflection);
}

void test_expo_preserves_full_deflection() {
    TEST_ASSERT_EQUAL(MAX_RAW, conditionAxis(MAX_RAW, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 800, RESPONSE_FULL_SCALE));
    TEST_ASSERT_EQUAL(MIN_RAW, conditionAxis(MIN_RAW, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 800, RESPONSE_FULL_SCALE));
}

// --- Rate caps the maximum output ---

void test_rate_scales_full_deflection() {
    // rate 500 => 50% of span at full stick
    TEST_ASSERT_EQUAL(CENTER + SPAN / 2, conditionAxis(MAX_RAW, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 0, 500));
    TEST_ASSERT_EQUAL(CENTER - SPAN / 2, conditionAxis(MIN_RAW, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 0, 500));
}

void test_shape_deflection_endpoint_preserved_with_rate_full() {
    TEST_ASSERT_EQUAL(SPAN, shapeDeflection(SPAN, SPAN, 600, RESPONSE_FULL_SCALE));
}

// --- Clamping ---

void test_out_of_range_input_clamped() {
    TEST_ASSERT_EQUAL(linearAxis(MIN_RAW), linearAxis(MIN_RAW - 500));
    TEST_ASSERT_EQUAL(linearAxis(MAX_RAW), linearAxis(MAX_RAW + 500));
}

void test_zero_span_is_zero() {
    TEST_ASSERT_EQUAL(0, shapeDeflection(100, 0, 0, RESPONSE_FULL_SCALE));
}

// --- Monotonicity across the full range (with a curve applied) ---

void test_monotonic_non_decreasing_with_curve() {
    int prev = conditionAxis(MIN_RAW, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 500, 800);
    for (int raw = MIN_RAW + 1; raw <= MAX_RAW; raw++) {
        int curr = conditionAxis(raw, CENTER, MIN_RAW, MAX_RAW, DEADZONE, 500, 800);
        TEST_ASSERT_GREATER_OR_EQUAL(prev, curr);
        prev = curr;
    }
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_within_deadzone_returns_center);
    RUN_TEST(test_deadzone_edge_returns_center);
    RUN_TEST(test_just_outside_deadzone_not_center);
    RUN_TEST(test_resting_jitter_conditions_to_center);
    RUN_TEST(test_linear_is_identity_outside_deadzone);
    RUN_TEST(test_shape_deflection_identity_fast_path);
    RUN_TEST(test_expo_softens_near_center);
    RUN_TEST(test_expo_preserves_full_deflection);
    RUN_TEST(test_rate_scales_full_deflection);
    RUN_TEST(test_shape_deflection_endpoint_preserved_with_rate_full);
    RUN_TEST(test_out_of_range_input_clamped);
    RUN_TEST(test_zero_span_is_zero);
    RUN_TEST(test_monotonic_non_decreasing_with_curve);
    return UNITY_END();
}
