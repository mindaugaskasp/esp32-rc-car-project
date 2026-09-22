#include "ChannelSync.h"
#include "config/controller/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include "ui/WifiScanScreen.h"
#include "comm/ChannelScanner.h"
#include "comm/ChannelAdvertiser.h"
#include <Arduino.h>

// even if the two boards aren't powered on at exactly the same moment. Keep in
// sync with CHANNEL_SYNC_TIMEOUT_MS in main_receiver.cpp.
static constexpr unsigned long CHANNEL_BROADCAST_MS = 12000;

ChannelScanResult scanChannel() {
    // Scan for the least congested 2.4GHz channel (runs before ESP-NOW init).
    return scanForBestChannel([]() {
        wifiScanScreen.showScanning();
    });
}

void broadcastChannelToReceiver(const ChannelScanResult& scanResult) {
    // Broadcast the chosen channel on ADVERTISEMENT_CHANNEL so the receiver can sync.
    // Display scan results while broadcasting — both happen simultaneously.
    initChannelBroadcast();
    unsigned long scanShownAt = millis();
    while (millis() - scanShownAt < CHANNEL_BROADCAST_MS) {
        sendChannelAdvertisement(scanResult.bestChannel);
        int secsLeft = static_cast<int>((CHANNEL_BROADCAST_MS - (millis() - scanShownAt)) / 1000);
        wifiScanScreen.showResult(scanResult, secsLeft, /*broadcasting=*/true);
        if (readButton(THROTTLE_SW_PIN) || readButton(STEERING_SW_PIN)) break;
        delay(200);
    }
    stopChannelBroadcast();
}

// The receiver alternates 1.5s listening on ADVERTISEMENT_CHANNEL with 1.5s back on
// the operational channel, so a single advertisement packet would usually miss it.
// Advertise for longer than one full listen/serve cycle to guarantee an overlap.
static constexpr unsigned long READVERTISE_AFTER_LOSS_MS = 5000;
static constexpr unsigned long READVERTISE_WINDOW_MS = 3500;

void updateChannelReadvertise(unsigned long now, uint8_t operationalChannel, bool telemetryReceived) {
    static unsigned long lastTelemetryMs = 0;
    static unsigned long readvertiseStartedMs = 0;
    static bool readvertising = false;

    if (lastTelemetryMs == 0) lastTelemetryMs = now;

    if (readvertising) {
        sendChannelAdvertisement(operationalChannel);
        if (now - readvertiseStartedMs >= READVERTISE_WINDOW_MS) {
            stopChannelBroadcast();
            applyWifiChannel(operationalChannel);
            readvertising = false;
            lastTelemetryMs = now; // restart the countdown before trying again
        }
        return;
    }

    if (telemetryReceived) {
        lastTelemetryMs = now;
        return;
    }
    // Only fires once the link is already dead, so leaving the operational channel
    // costs nothing: the receiver's watchdog is holding the ESC at neutral anyway.
    if (now - lastTelemetryMs >= READVERTISE_AFTER_LOSS_MS) {
        initChannelBroadcast();
        readvertiseStartedMs = now;
        readvertising = true;
    }
}
