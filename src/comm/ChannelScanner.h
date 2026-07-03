#pragma once
#include <stdint.h>

constexpr uint8_t WIFI_2GHZ_CHANNEL_COUNT = 13; // channels scanned for congestion info
constexpr uint8_t WIFI_2GHZ_MAX_SELECTABLE_CHANNEL = 11; // channels 12-13 are illegal in some regulatory domains (e.g. US) — never auto-select them

struct ChannelScanResult {
    uint8_t bestChannel; // 1–13
    int channelScore; // congestion score; lower = less congested
    uint8_t channelApCount; // APs detected on the best channel
    uint8_t totalApCount; // total APs found across all channels
    const char* qualityLabel;    // "Excellent" / "Good" / "Fair" / "Poor"
};

// Scans all 2.4GHz channels and returns the least congested one.
// Switches WiFi to WIFI_STA mode internally. MUST be called BEFORE initEspNow() —
// an active ESP-NOW session makes the scan return 0 APs on every channel.
// progressCallback is invoked once per channel during the scan (drives the
// scan-screen spinner); the scan is synchronous, one channel at a time.
ChannelScanResult scanForBestChannel(void (*progressCallback)() = nullptr);

// Sets the active WiFi channel for ESP-NOW. Call before addPeer().
void applyWifiChannel(uint8_t channel);

uint8_t getSelectedChannel();
