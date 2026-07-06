#include <unity.h>
#include "drivers/hall/SpeedLogic.h"

// Tests use explicit wheelDiameterMm / gearRatio arguments so they stay valid no
// matter what VehicleConfig.h defaults are set to for a given car.

void setUp() {}
void tearDown() {}

// --- Known mapped value ---

void test_known_value() {
    // 8000 motor RPM, gear 8 -> 1000 wheel RPM.
    // circumference = PI * 65mm / 1000 = 0.204204 m
    // km/h = 1000 * 0.204204 * 60 / 1000 = 12.252
    float kmh = motorRpmToKmh(8000, /*wheelDiameterMm=*/65.0f, /*gearRatio=*/8.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 12.252f, kmh);
}

void test_zero_rpm_is_zero() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, motorRpmToKmh(0, 65.0f, 8.0f));
}

// --- Scaling relationships (exact ratios, robust to constant changes) ---

void test_double_gear_ratio_halves_speed() {
    float baseline = motorRpmToKmh(8000, 65.0f, 8.0f);
    float doubled = motorRpmToKmh(8000, 65.0f, 16.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, baseline / 2.0f, doubled);
}

void test_double_wheel_diameter_doubles_speed() {
    float baseline = motorRpmToKmh(8000, 65.0f, 8.0f);
    float doubled = motorRpmToKmh(8000, 130.0f, 8.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, baseline * 2.0f, doubled);
}

void test_higher_rpm_is_faster() {
    TEST_ASSERT_TRUE(motorRpmToKmh(10000, 65.0f, 8.0f) > motorRpmToKmh(5000, 65.0f, 8.0f));
}

void test_speed_is_linear_in_rpm() {
    // Doubling RPM doubles speed (the map is purely proportional).
    float single = motorRpmToKmh(5000, 65.0f, 8.0f);
    float doubled = motorRpmToKmh(10000, 65.0f, 8.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, single * 2.0f, doubled);
}

// --- Degenerate inputs are guarded (no divide-by-zero, no garbage) ---

void test_zero_gear_ratio_reports_zero() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, motorRpmToKmh(8000, 65.0f, 0.0f));
}

void test_negative_gear_ratio_reports_zero() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, motorRpmToKmh(8000, 65.0f, -8.0f));
}

void test_zero_wheel_diameter_reports_zero() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, motorRpmToKmh(8000, 0.0f, 8.0f));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_known_value);
    RUN_TEST(test_zero_rpm_is_zero);
    RUN_TEST(test_double_gear_ratio_halves_speed);
    RUN_TEST(test_double_wheel_diameter_doubles_speed);
    RUN_TEST(test_higher_rpm_is_faster);
    RUN_TEST(test_speed_is_linear_in_rpm);
    RUN_TEST(test_zero_gear_ratio_reports_zero);
    RUN_TEST(test_negative_gear_ratio_reports_zero);
    RUN_TEST(test_zero_wheel_diameter_reports_zero);
    return UNITY_END();
}
