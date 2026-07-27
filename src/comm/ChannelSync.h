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
// Mirrors the receiver's single-call waitForChannelAdvertisement().

// Scan for the least congested 2.4GHz channel, showing progress on the WiFi-scan
// screen. MUST be called BEFORE initEspNow() (see ChannelScanner.h).
ChannelScanResult scanChannel();

// Advertise the chosen channel to the receiver on ADVERTISEMENT_CHANNEL for a fixed
// window, showing scan results. Requires initEspNow() to have run first.
void broadcastChannelToReceiver(const ChannelScanResult& scanResult);

// Call every loop() tick with the operational channel and whether telemetry arrived
// this tick. After the link has been silent for READVERTISE_AFTER_LOSS_MS it re-runs
// a short channel advertisement so a receiver that booted late (or lost sync) can
// find us again without the transmitter being rebooted. Non-blocking: it hops to
// ADVERTISEMENT_CHANNEL, advertises across subsequent ticks, then returns to the
// operational channel.
void updateChannelReadvertise(unsigned long now, uint8_t operationalChannel, bool telemetryReceived);
