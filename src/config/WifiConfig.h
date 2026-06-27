#pragma once

#include <Arduino.h>

// Set the peer MAC address for the receiver board here.
// Use the receiver's printed MAC address from Serial monitor.
static const uint8_t RECEIVER_MAC[6] = {0x30, 0x76, 0xF5, 0xA6, 0x55, 0x58};

// Optional: retain the transmitter MAC if you ever need it.
static const uint8_t TRANSMITTER_MAC[6] = {0x68, 0x09, 0x47, 0x9C, 0xE4, 0xE4};
