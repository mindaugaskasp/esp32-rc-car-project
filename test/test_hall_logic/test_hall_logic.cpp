#include <unity.h>
#include "drivers/hall/HallLogic.h"

// Config values (from ControlConfig.h):
//   HALL_PULSES_PER_REV=2, HALL_MIN_PULSES_FOR_RPM=6, HALL_RPM_INTERVAL_MS=150

void setUp() {}
void tearDown() {}

// --- Idle-noise gate: below the minimum pulse count reports zero ---

void test_below_min_pulses_reports_zero() {
    // 5 pulses with default min of 6 -> gated to 0
    TEST_ASSERT_EQUAL(0, computeMotorRpm(5, 150));
}

void test_zero_pulses_reports_zero() {
    TEST_ASSERT_EQUAL(0, computeMotorRpm(0, 150));
}

void test_exactly_min_pulses_reports_nonzero() {
    // 6 pulses over 150 ms, 2 pulses/rev: 6*60000/(2*150) = 360000/300 = 1200
    TEST_ASSERT_EQUAL(1200, computeMotorRpm(6, 150));
}

// --- Known mapped values (integer arithmetic) ---

void test_known_value_default_config() {
    // 100 pulses over 150 ms, 2 pulses/rev: 100*60000/(2*150) = 6,000,000/300 = 20000
    TEST_ASSERT_EQUAL(20000, computeMotorRpm(100, 150));
}

void test_single_pulse_per_rev() {
    // 10 pulses over 100 ms, 1 pulse/rev, min 1: 10*60000/(1*100) = 6000
    TEST_ASSERT_EQUAL(6000, computeMotorRpm(10, 100, /*pulsesPerRev=*/1, /*minPulses=*/1));
}

void test_integer_truncation_matches_driver() {
    // 10 pulses over 175 ms, 2 pulses/rev: 10*60000/(2*175) = 600000/350 = 1714.28 -> 1714
    TEST_ASSERT_EQUAL(1714, computeMotorRpm(10, 175, /*pulsesPerRev=*/2, /*minPulses=*/1));
}

// --- pulses-per-rev scaling ---

void test_more_poles_halves_rpm() {
    // Same pulses/window: doubling pulses-per-rev halves the reported RPM.
    int oneMagnet  = computeMotorRpm(50, 150, /*pulsesPerRev=*/1, /*minPulses=*/1);
    int twoMagnets = computeMotorRpm(50, 150, /*pulsesPerRev=*/2, /*minPulses=*/1);
    TEST_ASSERT_EQUAL(oneMagnet / 2, twoMagnets);
}

// --- Degenerate inputs are guarded (no divide-by-zero, no garbage) ---

void test_zero_elapsed_reports_zero() {
    TEST_ASSERT_EQUAL(0, computeMotorRpm(100, 0));
}

void test_zero_pulses_per_rev_reports_zero() {
    // Would divide by zero if unguarded.
    TEST_ASSERT_EQUAL(0, computeMotorRpm(100, 150, /*pulsesPerRev=*/0, /*minPulses=*/1));
}

void test_negative_pulses_per_rev_reports_zero() {
    TEST_ASSERT_EQUAL(0, computeMotorRpm(100, 150, /*pulsesPerRev=*/-1, /*minPulses=*/1));
}

// --- Longer window at the same rate yields the same RPM ---

void test_rate_invariant_over_window_length() {
    // 20 pulses/150ms and 40 pulses/300ms are the same pulse rate -> same RPM.
    int shortWindow = computeMotorRpm(20, 150, /*pulsesPerRev=*/2, /*minPulses=*/1);
    int longWindow  = computeMotorRpm(40, 300, /*pulsesPerRev=*/2, /*minPulses=*/1);
    TEST_ASSERT_EQUAL(shortWindow, longWindow);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_below_min_pulses_reports_zero);
    RUN_TEST(test_zero_pulses_reports_zero);
    RUN_TEST(test_exactly_min_pulses_reports_nonzero);
    RUN_TEST(test_known_value_default_config);
    RUN_TEST(test_single_pulse_per_rev);
    RUN_TEST(test_integer_truncation_matches_driver);
    RUN_TEST(test_more_poles_halves_rpm);
    RUN_TEST(test_zero_elapsed_reports_zero);
    RUN_TEST(test_zero_pulses_per_rev_reports_zero);
    RUN_TEST(test_negative_pulses_per_rev_reports_zero);
    RUN_TEST(test_rate_invariant_over_window_length);
    return UNITY_END();
}
