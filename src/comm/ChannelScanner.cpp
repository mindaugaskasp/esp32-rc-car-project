#include "ChannelScanner.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <drivers/debug/DebugLogger.h>

static uint8_t selectedChannel = 1;

// Pre-scan radio settle, split into frames so the progress spinner animates.
// SCAN_SETTLE_FRAMES * SCAN_SETTLE_FRAME_MS ≈ the original 100ms settle time.
static constexpr int SCAN_SETTLE_FRAMES = 4;
static constexpr int SCAN_SETTLE_FRAME_MS = 25;

// Per-channel active-scan dwell. The scan runs one channel at a time (not one
// blocking all-channel call) so the spinner can tick between channels; total scan
// time ≈ SCAN_MS_PER_CHANNEL * WIFI_2GHZ_CHANNEL_COUNT, same as a single scan.
static constexpr uint32_t SCAN_MS_PER_CHANNEL = 250;

// Congestion score constants
static constexpr int RSSI_SCORE_BASE = 100; // offsets negative RSSI into a positive contribution
static constexpr int QUALITY_THRESHOLD_GOOD = 30;
static constexpr int QUALITY_THRESHOLD_FAIR = 70;

static const char* qualityForScore(int score) {
    if (score == 0) return "Excellent";
    if (score < QUALITY_THRESHOLD_GOOD) return "Good";
    if (score < QUALITY_THRESHOLD_FAIR) return "Fair";
    return "Poor";
}

ChannelScanResult scanForBestChannel(void (*progressCallback)()) {
    // Must run BEFORE esp_now_init(): an active ESP-NOW session suppresses scan
    // results. Put the radio in station mode and drop any prior association, then
    // let it settle so the first scan after boot isn't empty.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Let the radio settle before the first scan, animating the progress callback
    // across the settle so the spinner visibly turns instead of freezing on a
    // single frame. Keep the total settle near the original 100ms.
    for (int settleFrame = 0; settleFrame < SCAN_SETTLE_FRAMES; settleFrame++) {
        if (progressCallback) progressCallback();
        delay(SCAN_SETTLE_FRAME_MS);
    }

    int scores[WIFI_2GHZ_CHANNEL_COUNT + 1] = {}; // index 1–13; 0 unused
    int counts[WIFI_2GHZ_CHANNEL_COUNT + 1] = {};
    int totalAPs = 0;

    // Scan one channel at a time. Each single-channel scan is a short synchronous
    // call (as reliable as the old all-channel scan — no flaky async polling), and
    // ticking the spinner before each keeps the "Please wait" animation turning
    // across the whole scan instead of freezing on one blocking call. Scoring is
    // unchanged: APs are still bucketed by channel and summed.
    for (int channelNumber = 1; channelNumber <= WIFI_2GHZ_CHANNEL_COUNT; channelNumber++) {
        if (progressCallback) progressCallback();

        // show_hidden=true catches hidden SSIDs; last arg pins the scan to one channel.
        int numAPs = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true,
                                       /*passive=*/false, SCAN_MS_PER_CHANNEL,
                                       static_cast<uint8_t>(channelNumber));
        if (numAPs < 0) numAPs = 0; // negative = WIFI_SCAN_FAILED → treat as no APs

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
        totalAPs += numAPs;
        WiFi.scanDelete();
    }
    debugLogger.logf("[SCAN] scanned %d channels, %d APs total", WIFI_2GHZ_CHANNEL_COUNT, totalAPs);

    for (int channelNumber = 1; channelNumber <= WIFI_2GHZ_CHANNEL_COUNT; channelNumber++) {
        debugLogger.logf("[SCAN] Ch%2d  score=%3d  APs=%d", channelNumber, scores[channelNumber], counts[channelNumber]);
    }

    // Lowest score wins; lowest channel number breaks ties (deterministic).
    // Only channels 1-11 are eligible for selection — see WIFI_2GHZ_MAX_SELECTABLE_CHANNEL.
    int bestChannel = 1;
    for (int channelNumber = 2; channelNumber <= WIFI_2GHZ_MAX_SELECTABLE_CHANNEL; channelNumber++) {
        if (scores[channelNumber] < scores[bestChannel]) {
            bestChannel = channelNumber;
        }
    }

    selectedChannel = static_cast<uint8_t>(bestChannel);

    ChannelScanResult result;
    result.bestChannel = selectedChannel;
    result.channelScore = scores[bestChannel];
    result.channelApCount = static_cast<uint8_t>(counts[bestChannel] > 255 ? 255 : counts[bestChannel]);
    result.totalApCount = static_cast<uint8_t>(totalAPs > 255 ? 255 : totalAPs);
    result.qualityLabel = qualityForScore(result.channelScore);

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
