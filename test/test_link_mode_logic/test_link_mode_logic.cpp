#include <unity.h>
#include "comm/LinkModeLogic.h"

void setUp() {}
void tearDown() {}

void test_wire_round_trip() {
    TEST_ASSERT_EQUAL_UINT8(0, linkModeToWire(LinkPhyMode::Standard));
    TEST_ASSERT_EQUAL_UINT8(1, linkModeToWire(LinkPhyMode::LongRange));
    TEST_ASSERT_TRUE(linkModeFromWire(0) == LinkPhyMode::Standard);
    TEST_ASSERT_TRUE(linkModeFromWire(1) == LinkPhyMode::LongRange);
}

void test_wire_unknown_value_is_standard() {
    // Any unexpected byte must decode to the safe fallback, never a stray LR.
    TEST_ASSERT_TRUE(linkModeFromWire(2) == LinkPhyMode::Standard);
    TEST_ASSERT_TRUE(linkModeFromWire(255) == LinkPhyMode::Standard);
}

void test_toggle() {
    TEST_ASSERT_TRUE(toggledLinkMode(LinkPhyMode::Standard) == LinkPhyMode::LongRange);
    TEST_ASSERT_TRUE(toggledLinkMode(LinkPhyMode::LongRange) == LinkPhyMode::Standard);
}

void test_rx_adopts_only_on_difference() {
    TEST_ASSERT_TRUE(rxShouldAdopt(LinkPhyMode::LongRange, LinkPhyMode::Standard));
    TEST_ASSERT_TRUE(rxShouldAdopt(LinkPhyMode::Standard, LinkPhyMode::LongRange));
    TEST_ASSERT_FALSE(rxShouldAdopt(LinkPhyMode::Standard, LinkPhyMode::Standard));
    TEST_ASSERT_FALSE(rxShouldAdopt(LinkPhyMode::LongRange, LinkPhyMode::LongRange));
}

void test_tx_adopts_only_after_rx_confirms() {
    // Desired LR, still on Standard, RX now reports LR -> switch.
    TEST_ASSERT_TRUE(txShouldAdopt(LinkPhyMode::LongRange, LinkPhyMode::LongRange, LinkPhyMode::Standard));
    // RX has not moved yet -> hold.
    TEST_ASSERT_FALSE(txShouldAdopt(LinkPhyMode::Standard, LinkPhyMode::LongRange, LinkPhyMode::Standard));
    // Already matched -> nothing to do.
    TEST_ASSERT_FALSE(txShouldAdopt(LinkPhyMode::LongRange, LinkPhyMode::LongRange, LinkPhyMode::LongRange));
    // Toggling back to Standard, RX reports Standard -> switch.
    TEST_ASSERT_TRUE(txShouldAdopt(LinkPhyMode::Standard, LinkPhyMode::Standard, LinkPhyMode::LongRange));
}

void test_no_revert_from_standard() {
    TEST_ASSERT_FALSE(shouldRevertToStandard(LinkPhyMode::Standard, false, 100000, 800, 4000));
    TEST_ASSERT_FALSE(shouldRevertToStandard(LinkPhyMode::Standard, true, 100000, 800, 4000));
}

void test_revert_unconfirmed_uses_short_timeout() {
    // Unconfirmed switch: reverts at the short timeout, not before.
    TEST_ASSERT_FALSE(shouldRevertToStandard(LinkPhyMode::LongRange, false, 799, 800, 4000));
    TEST_ASSERT_TRUE(shouldRevertToStandard(LinkPhyMode::LongRange, false, 800, 800, 4000));
}

void test_revert_confirmed_uses_long_timeout() {
    // Confirmed LR link tolerates ordinary gaps up to the long timeout.
    TEST_ASSERT_FALSE(shouldRevertToStandard(LinkPhyMode::LongRange, true, 3999, 800, 4000));
    TEST_ASSERT_TRUE(shouldRevertToStandard(LinkPhyMode::LongRange, true, 4000, 800, 4000));
}

void test_give_up_only_while_link_alive_and_mismatched() {
    // Alive link, request unacked past the deadline -> give up.
    TEST_ASSERT_TRUE(txShouldGiveUpRequest(LinkPhyMode::LongRange, LinkPhyMode::Standard, true, 3000, 3000));
    // Not yet at the deadline.
    TEST_ASSERT_FALSE(txShouldGiveUpRequest(LinkPhyMode::LongRange, LinkPhyMode::Standard, true, 2999, 3000));
    // Link dead -> the revert path handles it, not give-up.
    TEST_ASSERT_FALSE(txShouldGiveUpRequest(LinkPhyMode::LongRange, LinkPhyMode::Standard, false, 5000, 3000));
    // Already settled -> nothing to give up.
    TEST_ASSERT_FALSE(txShouldGiveUpRequest(LinkPhyMode::Standard, LinkPhyMode::Standard, true, 5000, 3000));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_wire_round_trip);
    RUN_TEST(test_wire_unknown_value_is_standard);
    RUN_TEST(test_toggle);
    RUN_TEST(test_rx_adopts_only_on_difference);
    RUN_TEST(test_tx_adopts_only_after_rx_confirms);
    RUN_TEST(test_no_revert_from_standard);
    RUN_TEST(test_revert_unconfirmed_uses_short_timeout);
    RUN_TEST(test_revert_confirmed_uses_long_timeout);
    RUN_TEST(test_give_up_only_while_link_alive_and_mismatched);
    return UNITY_END();
}
