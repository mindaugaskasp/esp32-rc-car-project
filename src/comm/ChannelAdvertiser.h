#pragma once
#include <stdint.h>

// Fixed channel used only for the startup handshake.
// Both devices switch to this channel briefly so the receiver can learn
// the transmitter's chosen operational channel.
constexpr uint8_t ADVERTISEMENT_CHANNEL = 1;

// ── Transmitter side ──────────────────────────────────────────────────────────

// Switch to ADVERTISEMENT_CHANNEL and set up the broadcast peer. Call after initEspNow().
void initChannelBroadcast();

// Send one channel advertisement packet. Call repeatedly in a loop.
void sendChannelAdvertisement(uint8_t channel);

// Remove the broadcast peer and finish the handshake phase.
void stopChannelBroadcast();

// ── Receiver side ─────────────────────────────────────────────────────────────

// Switch to ADVERTISEMENT_CHANNEL and block until a channel advertisement is received from
// expectedTransmitterMac, or timeoutMs elapses. Returns the advertised channel, or
// fallbackChannel on timeout. Packets from other senders are ignored.
uint8_t receiveChannelAdvertisement(const uint8_t* expectedTransmitterMac, uint32_t timeoutMs, uint8_t fallbackChannel = 6);

// Returns true if (data, len) is a channel advertisement from expectedMac, writing the
// advertised channel to *outChannel. ISR-safe (no allocation, no Serial) so it can be
// called directly from an ESP-NOW receive callback — used for runtime channel resync
// (see VehicleCommandReceiver). A null expectedMac accepts any sender.
bool tryParseChannelAdvertisement(const uint8_t* mac, const uint8_t* data, int len,
                                  const uint8_t* expectedMac, uint8_t* outChannel);
