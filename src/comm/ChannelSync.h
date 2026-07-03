#pragma once
#include "comm/ChannelScanner.h"

// Transmitter-side startup channel handshake, split around ESP-NOW init because the
// WiFi scan must run before ESP-NOW is initialized while the broadcast needs it:
//
//   ChannelScanResult scan = scanChannel();      // BEFORE initEspNow()
//   initEspNow();
//   broadcastChannelToReceiver(scan);            // needs ESP-NOW
//   applyWifiChannel(scan.bestChannel);
//
// Mirrors the receiver's single-call receiveChannelAdvertisement().

// Scan for the least congested 2.4GHz channel, showing progress on the WiFi-scan
// screen. MUST be called BEFORE initEspNow() (see ChannelScanner.h).
ChannelScanResult scanChannel();

// Advertise the chosen channel to the receiver on ADVERTISEMENT_CHANNEL for a fixed
// window, showing scan results. Requires initEspNow() to have run first.
void broadcastChannelToReceiver(const ChannelScanResult& scanResult);
