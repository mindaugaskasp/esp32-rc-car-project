#include <unity.h>
#include "drivers/hall/HallLogic.h"

// Config values (from ControlConfig.h):
//   HALL_PULSES_PER_REV=2, HALL_MIN_PULSES_FOR_RPM=6, HALL_RPM_INTERVAL_MS=150
//   HALL_STOP_TIMEOUT_US=250000, HALL_RPM_SMOOTHING_NUM/DEN=1/2

void setUp() {}
void tearDown() {}

// --- computeMotorRpm: idle-noise gate below the minimum pulse count ---

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

// --- computeMotorRpm: known mapped values (integer arithmetic) ---

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

void test_more_poles_halves_rpm() {
    // Same pulses/window: doubling pulses-per-rev halves the reported RPM.
    int oneMagnet  = computeMotorRpm(50, 150, /*pulsesPerRev=*/1, /*minPulses=*/1);
    int twoMagnets = computeMotorRpm(50, 150, /*pulsesPerRev=*/2, /*minPulses=*/1);
    TEST_ASSERT_EQUAL(oneMagnet / 2, twoMagnets);
}

// --- computeMotorRpm: degenerate inputs are guarded (no divide-by-zero) ---

void test_zero_elapsed_reports_zero() {
    TEST_ASSERT_EQUAL(0, computeMotorRpm(100, 0));
}

void test_zero_pulses_per_rev_reports_zero() {
    TEST_ASSERT_EQUAL(0, computeMotorRpm(100, 150, /*pulsesPerRev=*/0, /*minPulses=*/1));
}

void test_negative_pulses_per_rev_reports_zero() {
    TEST_ASSERT_EQUAL(0, computeMotorRpm(100, 150, /*pulsesPerRev=*/-1, /*minPulses=*/1));
}

void test_rate_invariant_over_window_length() {
    // 20 pulses/150ms and 40 pulses/300ms are the same pulse rate -> same RPM.
    int shortWindow = computeMotorRpm(20, 150, /*pulsesPerRev=*/2, /*minPulses=*/1);
    int longWindow  = computeMotorRpm(40, 300, /*pulsesPerRev=*/2, /*minPulses=*/1);
    TEST_ASSERT_EQUAL(shortWindow, longWindow);
}

// --- rpmFromPulseInterval: low-speed timing path ---

void test_interval_known_value() {
    // 10 ms period, 2 pulses/rev: one rev = 20 ms -> 3000 RPM.
    // 60,000,000 / (10000 * 2) = 3000
    TEST_ASSERT_EQUAL(3000, rpmFromPulseInterval(10000));
}

void test_interval_single_pulse_per_rev() {
    // 10 ms period, 1 pulse/rev: one rev = 10 ms -> 6000 RPM.
    TEST_ASSERT_EQUAL(6000, rpmFromPulseInterval(10000, /*pulsesPerRev=*/1));
}

void test_interval_shorter_period_is_faster() {
    TEST_ASSERT_TRUE(rpmFromPulseInterval(5000) > rpmFromPulseInterval(10000));
}

void test_interval_zero_reports_zero() {
    TEST_ASSERT_EQUAL(0, rpmFromPulseInterval(0));
}

void test_interval_guards_pulses_per_rev() {
    TEST_ASSERT_EQUAL(0, rpmFromPulseInterval(10000, /*pulsesPerRev=*/0));
}

// --- selectWindowRpm: picks count-based, else fresh interval, else 0 ---

void test_select_uses_count_when_above_gate() {
    // 100 pulses over 150 ms clears the gate -> count-based 20000, interval ignored.
    int selected = selectWindowRpm(100, 150, /*intervalUs=*/10000, /*maxIntervalUs=*/250000);
    TEST_ASSERT_EQUAL(20000, selected);
}

void test_select_falls_back_to_interval_below_gate() {
    // 3 pulses (< min 6) -> count path returns 0, interval path used instead.
    int selected = selectWindowRpm(3, 150, /*intervalUs=*/10000, /*maxIntervalUs=*/250000);
    TEST_ASSERT_EQUAL(rpmFromPulseInterval(10000), selected);
}

void test_select_rejects_stale_interval() {
    // Below the gate and the last interval is older than the freshness bound:
    // treat as no reading rather than a phantom speed.
    int selected = selectWindowRpm(3, 150, /*intervalUs=*/300000, /*maxIntervalUs=*/250000);
    TEST_ASSERT_EQUAL(0, selected);
}

void test_select_zero_when_no_pulses_and_no_interval() {
    TEST_ASSERT_EQUAL(0, selectWindowRpm(0, 150, /*intervalUs=*/0, /*maxIntervalUs=*/250000));
}

// --- smoothRpm: EMA filter ---

void test_smooth_moves_toward_sample() {
    // previous 0, sample 1000, weight 1/2 -> 500.
    TEST_ASSERT_EQUAL(500, smoothRpm(0, 1000, 1, 2));
}

void test_smooth_converges_to_steady_sample() {
    int value = 0;
    for (int step = 0; step < 40; step++) {
        value = smoothRpm(value, 1000, 1, 2);
    }
    TEST_ASSERT_EQUAL(1000, value);
}

void test_smooth_rounds_small_step_instead_of_stalling() {
    // previous 100, sample 101, weight 1/2: delta 1 would truncate to 0 without
    // rounding, freezing the filter one count short. Rounding advances it.
    TEST_ASSERT_EQUAL(101, smoothRpm(100, 101, 1, 2));
}

void test_smooth_handles_falling_sample() {
    // previous 1000, sample 0, weight 1/2 -> 500 (symmetric to the rising case).
    TEST_ASSERT_EQUAL(500, smoothRpm(1000, 0, 1, 2));
}

void test_smooth_disabled_returns_sample() {
    TEST_ASSERT_EQUAL(1234, smoothRpm(0, 1234, 1, 0));
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
    RUN_TEST(test_interval_known_value);
    RUN_TEST(test_interval_single_pulse_per_rev);
    RUN_TEST(test_interval_shorter_period_is_faster);
    RUN_TEST(test_interval_zero_reports_zero);
    RUN_TEST(test_interval_guards_pulses_per_rev);
    RUN_TEST(test_select_uses_count_when_above_gate);
    RUN_TEST(test_select_falls_back_to_interval_below_gate);
    RUN_TEST(test_select_rejects_stale_interval);
    RUN_TEST(test_select_zero_when_no_pulses_and_no_interval);
    RUN_TEST(test_smooth_moves_toward_sample);
    RUN_TEST(test_smooth_converges_to_steady_sample);
    RUN_TEST(test_smooth_rounds_small_step_instead_of_stalling);
    RUN_TEST(test_smooth_handles_falling_sample);
    RUN_TEST(test_smooth_disabled_returns_sample);
    return UNITY_END();
}
