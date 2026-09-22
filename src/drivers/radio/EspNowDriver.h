#pragma once
#include <Arduino.h>
#include "comm/DataTypes.h"

void sendData(VehicleData data, const uint8_t* mac);

// Switch the WiFi PHY between Standard (802.11 b/g/n) and Long Range (LR). Both
// boards call this; the coordination that keeps them in step lives in
// comm/LinkModeController (transmitter) and VehicleCommandReceiver (receiver).
void applyLinkPhyMode(bool longRange);

// Sets the linkMode byte stamped onto every outgoing VehicleData in sendData().
// Kept as a driver-level cache so the send path carries the transmitter's desired
// PHY without the driver depending on the comm-layer negotiation state.
void setOutgoingLinkMode(uint8_t mode);
// Milliseconds since the last sendData() call. Used by the transmitter loop to
// decide when to emit a keep-alive heartbeat so the receiver can distinguish an
// idle stick from a lost link.
unsigned long millisSinceLastSend();
void initEspNow();
void addPeer(const uint8_t* macAddress);
void printMacAddress();

// Bytes needed to hold a MAC formatted as "AA:BB:CC:DD:EE:FF" plus the null
// terminator (17 characters + 1).
static constexpr size_t MAC_STRING_BUFFER_SIZE = 18;

// Formats a 6-byte MAC into out as "AA:BB:CC:DD:EE:FF". out must hold at least
// MAC_STRING_BUFFER_SIZE bytes.
void formatMac(char* out, const uint8_t* mac);