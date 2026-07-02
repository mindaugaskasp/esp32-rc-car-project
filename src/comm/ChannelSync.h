#pragma once
#include <stdint.h>

// Transmitter-side startup channel handshake: scans for the least congested
// 2.4GHz channel, advertises it to the receiver on ADVERTISEMENT_CHANNEL for a fixed
// window (showing scan progress on the WiFi-scan screen), then returns the
// chosen channel. Mirrors the receiver's single-call receiveChannelAdvertisement().
// The caller applies the result via applyWifiChannel(). Requires initEspNow()
// to have run first.
uint8_t syncChannelWithReceiver();
