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

// Per-channel congestion tallies. Index 1-13; slot 0 is unused so a channel
// number indexes directly.
struct ChannelScores {
    int score[WIFI_2GHZ_CHANNEL_COUNT + 1] = {};
    int apCount[WIFI_2GHZ_CHANNEL_COUNT + 1] = {};
    int totalApCount = 0;
};

// Put the radio in station mode, drop any prior association, and let it settle so
// the first scan after boot isn't empty. Must run BEFORE esp_now_init(): an active
// ESP-NOW session suppresses scan results. The progress callback is ticked across
// the settle so the spinner turns instead of freezing on one frame.
static void settleRadioForScan(void (*progressCallback)()) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    for (int settleFrame = 0; settleFrame < SCAN_SETTLE_FRAMES; settleFrame++) {
        if (progressCallback) progressCallback();
        delay(SCAN_SETTLE_FRAME_MS);
    }
}

// One short synchronous scan per channel — as reliable as a single all-channel
// scan without flaky async polling, and ticking the spinner between channels keeps
// the animation turning across what is otherwise one long blocking call.
static ChannelScores accumulateChannelScores(void (*progressCallback)()) {
    ChannelScores scores;

    for (int channelNumber = 1; channelNumber <= WIFI_2GHZ_CHANNEL_COUNT; channelNumber++) {
        if (progressCallback) progressCallback();

        // show_hidden=true catches hidden SSIDs; last arg pins the scan to one channel.
        int numAPs = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true,
                                       /*passive=*/false, SCAN_MS_PER_CHANNEL,
                                       static_cast<uint8_t>(channelNumber));
        if (numAPs < 0) numAPs = 0; // negative = WIFI_SCAN_FAILED → treat as no APs

        for (int apIndex = 0; apIndex < numAPs; apIndex++) {
            const int channel = WiFi.channel(apIndex);
            if (channel < 1 || channel > WIFI_2GHZ_CHANNEL_COUNT) continue;
            // Stronger signals count more: -30dBm → 70pts, -80dBm → 20pts, -100dBm → 0pts
            int contribution = RSSI_SCORE_BASE + WiFi.RSSI(apIndex);
            if (contribution < 0) contribution = 0;
            scores.score[channel] += contribution;
            scores.apCount[channel]++;
        }
        scores.totalApCount += numAPs;
        WiFi.scanDelete();
    }

    return scores;
}

// Lowest score wins; lowest channel number breaks ties, so the choice is
// deterministic. Only channels 1-11 are eligible — see WIFI_2GHZ_MAX_SELECTABLE_CHANNEL.
static int pickLeastBusyChannel(const ChannelScores& scores) {
    int bestChannel = 1;
    for (int channelNumber = 2; channelNumber <= WIFI_2GHZ_MAX_SELECTABLE_CHANNEL; channelNumber++) {
        if (scores.score[channelNumber] < scores.score[bestChannel]) {
            bestChannel = channelNumber;
        }
    }
    return bestChannel;
}

static void logChannelScores(const ChannelScores& scores) {
    debugLogger.logf("[SCAN] scanned %d channels, %d APs total",
                     WIFI_2GHZ_CHANNEL_COUNT, scores.totalApCount);
    for (int channelNumber = 1; channelNumber <= WIFI_2GHZ_CHANNEL_COUNT; channelNumber++) {
        debugLogger.logf("[SCAN] Ch%2d  score=%3d  APs=%d", channelNumber,
                         scores.score[channelNumber], scores.apCount[channelNumber]);
    }
}

static uint8_t clampToByte(int value) {
    return static_cast<uint8_t>(value > 255 ? 255 : value);
}

ChannelScanResult scanForBestChannel(void (*progressCallback)()) {
    settleRadioForScan(progressCallback);

    const ChannelScores scores = accumulateChannelScores(progressCallback);
    logChannelScores(scores);

    const int bestChannel = pickLeastBusyChannel(scores);
    selectedChannel = static_cast<uint8_t>(bestChannel);

    ChannelScanResult result;
    result.bestChannel = selectedChannel;
    result.channelScore = scores.score[bestChannel];
    result.channelApCount = clampToByte(scores.apCount[bestChannel]);
    result.totalApCount = clampToByte(scores.totalApCount);
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
