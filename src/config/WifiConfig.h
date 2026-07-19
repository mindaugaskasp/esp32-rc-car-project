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

// Set the peer MAC address for the receiver board here.
// Use the receiver's printed MAC address from Serial monitor.
static const uint8_t RECEIVER_MAC[MAC_ADDRESS_LENGTH] = {0x30, 0x76, 0xF5, 0xA6, 0x55, 0x58};

// Optional: retain the transmitter MAC if you ever need it.
static const uint8_t TRANSMITTER_MAC[MAC_ADDRESS_LENGTH] = {0x68, 0x09, 0x47, 0x9C, 0xE4, 0xE4};
