#include "ChannelScanner.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <drivers/debug/DebugLogger.h>

static uint8_t selectedChannel = 1;

// Congestion score constants
static constexpr int RSSI_SCORE_BASE      = 100;  // offsets negative RSSI into a positive contribution
static constexpr int QUALITY_THRESHOLD_GOOD = 30;
static constexpr int QUALITY_THRESHOLD_FAIR = 70;

static const char* qualityForScore(int score) {
    if (score == 0)                        return "Excellent";
    if (score < QUALITY_THRESHOLD_GOOD)    return "Good";
    if (score < QUALITY_THRESHOLD_FAIR)    return "Fair";
    return "Poor";
}

ChannelScanResult scanForBestChannel(void (*progressCallback)()) {
    // Async scan across all channels; show_hidden=true catches hidden SSIDs too
    WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/true);

    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
        if (progressCallback) progressCallback();
        delay(200);
    }

    int numAPs = WiFi.scanComplete();
    if (numAPs < 0) numAPs = 0;  // WIFI_SCAN_FAILED → treat as no APs

    int scores[WIFI_2GHZ_CHANNEL_COUNT + 1] = {};  // index 1–13; 0 unused
    int counts[WIFI_2GHZ_CHANNEL_COUNT + 1] = {};

    for (int apIndex = 0; apIndex < numAPs; apIndex++) {
        int channel = WiFi.channel(apIndex);
        if (channel < 1 || channel > WIFI_2GHZ_CHANNEL_COUNT) continue;
        int rssi = WiFi.RSSI(apIndex);
        // Stronger signals count more: -30dBm → 70pts, -80dBm → 20pts, -100dBm → 0pts
        int contribution = RSSI_SCORE_BASE + rssi;
        if (contribution < 0) contribution = 0;
        scores[channel] += contribution;
        counts[channel]++;
    }

    for (int channelNum = 1; channelNum <= WIFI_2GHZ_CHANNEL_COUNT; channelNum++) {
        debugLogger.logf("[SCAN] Ch%2d  score=%3d  APs=%d", channelNum, scores[channelNum], counts[channelNum]);
    }

    // Lowest score wins; lowest channel number breaks ties (deterministic).
    // Only channels 1-11 are eligible for selection — see WIFI_2GHZ_MAX_SELECTABLE_CHANNEL.
    int bestChannel = 1;
    for (int channelNum = 2; channelNum <= WIFI_2GHZ_MAX_SELECTABLE_CHANNEL; channelNum++) {
        if (scores[channelNum] < scores[bestChannel]) {
            bestChannel = channelNum;
        }
    }

    selectedChannel = (uint8_t)bestChannel;

    ChannelScanResult result;
    result.bestChannel    = selectedChannel;
    result.channelScore   = scores[bestChannel];
    result.channelApCount = (uint8_t)(counts[bestChannel] > 255 ? 255 : counts[bestChannel]);
    result.totalApCount   = (uint8_t)(numAPs > 255 ? 255 : numAPs);
    result.qualityLabel   = qualityForScore(result.channelScore);

    WiFi.scanDelete();

    debugLogger.logf("[SCAN] Best ch: %d  quality: %s  score: %d  APs on ch: %d / total: %d",
                     bestChannel, result.qualityLabel, result.channelScore,
                     result.channelApCount, result.totalApCount);

    return result;
}

void applyWifiChannel(uint8_t channel) {
    esp_err_t result = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (result != ESP_OK) {
        debugLogger.logf("[SCAN] Failed to set WiFi channel %d: %d", channel, result);
        return;
    }
    selectedChannel = channel;
    debugLogger.logf("[SCAN] WiFi channel set to %d", channel);
}

uint8_t getSelectedChannel() {
    return selectedChannel;
}
