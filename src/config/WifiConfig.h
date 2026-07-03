#pragma once

#include <Arduino.h>

// An IEEE 802 MAC address is always 6 bytes. Named so no memcpy/memcmp over a
// MAC inlines a bare 6, and so the address arrays below derive their size from it.
static const uint8_t MAC_ADDRESS_LENGTH = 6;

// Set the peer MAC address for the receiver board here.
// Use the receiver's printed MAC address from Serial monitor.
static const uint8_t RECEIVER_MAC[MAC_ADDRESS_LENGTH] = {0x30, 0x76, 0xF5, 0xA6, 0x55, 0x58};

// Optional: retain the transmitter MAC if you ever need it.
static const uint8_t TRANSMITTER_MAC[MAC_ADDRESS_LENGTH] = {0x68, 0x09, 0x47, 0x9C, 0xE4, 0xE4};
