#include <unity.h>
#include "comm/ArmingLogic.h"

void setUp() {}
void tearDown() {}

// --- Neutral classification ---

void test_exact_center_is_neutral() {
    TEST_ASSERT_TRUE(isNeutralThrottleCommand(THROTTLE_CENTER_RAW));
}

void test_band_edges_are_neutral() {
    TEST_ASSERT_TRUE(isNeutralThrottleCommand(THROTTLE_CENTER_RAW + ARM_NEUTRAL_THROTTLE_BAND));
    TEST_ASSERT_TRUE(isNeutralThrottleCommand(THROTTLE_CENTER_RAW - ARM_NEUTRAL_THROTTLE_BAND));
}

void test_just_outside_band_is_not_neutral() {
    TEST_ASSERT_FALSE(isNeutralThrottleCommand(THROTTLE_CENTER_RAW + ARM_NEUTRAL_THROTTLE_BAND + 1));
    TEST_ASSERT_FALSE(isNeutralThrottleCommand(THROTTLE_CENTER_RAW - ARM_NEUTRAL_THROTTLE_BAND - 1));
}

void test_full_throttle_and_full_brake_are_not_neutral() {
    TEST_ASSERT_FALSE(isNeutralThrottleCommand(THROTTLE_JOY_MAX));
    TEST_ASSERT_FALSE(isNeutralThrottleCommand(THROTTLE_JOY_MIN));
}

// --- Arming progression ---

void test_neutral_commands_arm_after_threshold() {
    int armingCount = 0;
    for (int frame = 0; frame < ARM_COMMAND_THRESHOLD; frame++) {
        TEST_ASSERT_FALSE(isArmed(armingCount));
        armingCount = nextArmingCount(armingCount, THROTTLE_CENTER_RAW);
    }
    TEST_ASSERT_TRUE(isArmed(armingCount));
}

void test_non_neutral_command_restarts_count() {
    int armingCount = nextArmingCount(0, THROTTLE_CENTER_RAW);
    TEST_ASSERT_EQUAL(1, armingCount);
    armingCount = nextArmingCount(armingCount, THROTTLE_JOY_MAX);
    TEST_ASSERT_EQUAL(0, armingCount);
}

void test_held_full_throttle_never_arms() {
    int armingCount = 0;
    for (int frame = 0; frame < 100; frame++) {
        armingCount = nextArmingCount(armingCount, THROTTLE_JOY_MAX);
    }
    TEST_ASSERT_FALSE(isArmed(armingCount));
}

void test_armed_state_persists_through_any_command() {
    int armingCount = ARM_COMMAND_THRESHOLD;
    armingCount = nextArmingCount(armingCount, THROTTLE_JOY_MAX);
    TEST_ASSERT_TRUE(isArmed(armingCount));
    armingCount = nextArmingCount(armingCount, THROTTLE_JOY_MIN);
    TEST_ASSERT_TRUE(isArmed(armingCount));
    armingCount = nextArmingCount(armingCount, THROTTLE_CENTER_RAW);
    TEST_ASSERT_TRUE(isArmed(armingCount));
}

// --- Dropout-at-throttle scenario: the reconnect sequence must pass through
// neutral before propulsion resumes ---

void test_reconnect_at_throttle_requires_stick_home_first() {
    int armingCount = 0; // reset by the packet-loss watchdog
    armingCount = nextArmingCount(armingCount, THROTTLE_JOY_MAX); // stick still held
    armingCount = nextArmingCount(armingCount, THROTTLE_JOY_MAX);
    armingCount = nextArmingCount(armingCount, THROTTLE_JOY_MAX);
    TEST_ASSERT_FALSE(isArmed(armingCount));

    armingCount = nextArmingCount(armingCount, THROTTLE_CENTER_RAW); // stick released
    armingCount = nextArmingCount(armingCount, THROTTLE_CENTER_RAW);
    TEST_ASSERT_TRUE(isArmed(armingCount));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_exact_center_is_neutral);
    RUN_TEST(test_band_edges_are_neutral);
    RUN_TEST(test_just_outside_band_is_not_neutral);
    RUN_TEST(test_full_throttle_and_full_brake_are_not_neutral);
    RUN_TEST(test_neutral_commands_arm_after_threshold);
    RUN_TEST(test_non_neutral_command_restarts_count);
    RUN_TEST(test_held_full_throttle_never_arms);
    RUN_TEST(test_armed_state_persists_through_any_command);
    RUN_TEST(test_reconnect_at_throttle_requires_stick_home_first);
    return UNITY_END();
}
