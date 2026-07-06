#include <unity.h>
#include "app/SessionStatsLogic.h"

void setUp() {}
void tearDown() {}

// --- accumulateSession: distance / time / average / max ---

void test_fresh_stats_are_zero() {
    SessionStats stats{};
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, stats.distanceKm);
    TEST_ASSERT_EQUAL_UINT32(0, stats.elapsedMs);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, stats.maxSpeedKmh);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, stats.avgSpeedKmh);
}

void test_one_hour_at_constant_speed_covers_that_distance() {
    SessionStats stats{};
    accumulateSession(stats, 10.0f, 3600000); // 10 km/h for one hour
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, stats.distanceKm);
    TEST_ASSERT_EQUAL_UINT32(3600000, stats.elapsedMs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, stats.avgSpeedKmh);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, stats.maxSpeedKmh);
}

void test_distance_accumulates_over_slices() {
    SessionStats stats{};
    for (int slice = 0; slice < 3600; slice++) {
        accumulateSession(stats, 20.0f, 1000); // 20 km/h, 1s slices, one hour total
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, stats.distanceKm);
    TEST_ASSERT_EQUAL_UINT32(3600000, stats.elapsedMs);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, stats.avgSpeedKmh);
}

void test_max_tracks_the_peak() {
    SessionStats stats{};
    accumulateSession(stats, 5.0f, 1000);
    accumulateSession(stats, 12.0f, 1000);
    accumulateSession(stats, 3.0f, 1000);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, stats.maxSpeedKmh);
}

void test_average_reflects_idle_time() {
    SessionStats stats{};
    accumulateSession(stats, 10.0f, 1800000); // 30 min moving at 10 km/h -> 5 km
    accumulateSession(stats, 0.0f, 1800000); // 30 min stopped
    // 5 km over one hour -> 5 km/h average
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, stats.distanceKm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, stats.avgSpeedKmh);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, stats.maxSpeedKmh);
}

void test_negative_speed_clamped_to_zero() {
    SessionStats stats{};
    accumulateSession(stats, -8.0f, 1000);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, stats.distanceKm);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, stats.maxSpeedKmh);
}

// --- SpeedHistory ring buffer ---

void test_history_starts_empty() {
    SpeedHistory history{};
    TEST_ASSERT_EQUAL_INT(0, history.count);
}

void test_history_records_in_order() {
    SpeedHistory history{};
    pushSpeedSample(history, 1.0f);
    pushSpeedSample(history, 2.0f);
    pushSpeedSample(history, 3.0f);
    TEST_ASSERT_EQUAL_INT(3, history.count);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, speedHistoryAt(history, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, speedHistoryAt(history, 2));
}

void test_history_count_saturates_at_capacity() {
    SpeedHistory history{};
    for (int sample = 0; sample < SPEED_HISTORY_CAPACITY + 50; sample++) {
        pushSpeedSample(history, static_cast<float>(sample));
    }
    TEST_ASSERT_EQUAL_INT(SPEED_HISTORY_CAPACITY, history.count);
}

void test_history_scrolls_oldest_out_when_full() {
    SpeedHistory history{};
    for (int sample = 0; sample < SPEED_HISTORY_CAPACITY + 3; sample++) {
        pushSpeedSample(history, static_cast<float>(sample));
    }
    // The three earliest samples (0,1,2) were overwritten; oldest retained is 3,
    // newest is the last value pushed.
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, speedHistoryAt(history, 0));
    float newest = static_cast<float>(SPEED_HISTORY_CAPACITY + 2);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, newest, speedHistoryAt(history, history.count - 1));
}

void test_history_negative_sample_clamped() {
    SpeedHistory history{};
    pushSpeedSample(history, -5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, speedHistoryAt(history, 0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fresh_stats_are_zero);
    RUN_TEST(test_one_hour_at_constant_speed_covers_that_distance);
    RUN_TEST(test_distance_accumulates_over_slices);
    RUN_TEST(test_max_tracks_the_peak);
    RUN_TEST(test_average_reflects_idle_time);
    RUN_TEST(test_negative_speed_clamped_to_zero);
    RUN_TEST(test_history_starts_empty);
    RUN_TEST(test_history_records_in_order);
    RUN_TEST(test_history_count_saturates_at_capacity);
    RUN_TEST(test_history_scrolls_oldest_out_when_full);
    RUN_TEST(test_history_negative_sample_clamped);
    return UNITY_END();
}
