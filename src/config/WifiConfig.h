#pragma once

#include <Arduino.h>

// An IEEE 802 MAC address is always 6 bytes. Named so no memcpy/memcmp over a
// MAC inlines a bare 6, and so the address arrays below derive their size from it.
static const uint8_t MAC_ADDRESS_LENGTH = 6;

// Compile-time BOOT PHY for the ESP-NOW link. false = Standard 802.11 b/g/n
// ("usual mode"); true = Espressif-proprietary Long Range (LR), which trades data
// rate for sensitivity/range (up to ~1 km line-of-sight) and only works
// ESP32-to-ESP32. Long Range can also be toggled live from the transmitter's Link
// Mode screen; that runtime state is session-only and resets to this default on
// every boot, so both boards always power up agreeing on the PHY.
static const bool ESP_NOW_LONG_RANGE = false;

static const uint8_t RECEIVER_MAC[MAC_ADDRESS_LENGTH] = {0x1C, 0xDB, 0xD4, 0x9D, 0x8D, 0xE0};

static const uint8_t TRANSMITTER_MAC[MAC_ADDRESS_LENGTH] = {0xE0, 0x72, 0xA1, 0xD2, 0x42, 0xF4};
