#include "ChannelSync.h"
#include "config/controller/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include "ui/WifiScanScreen.h"
#include "comm/ChannelScanner.h"
#include "comm/ChannelAdvertiser.h"
#include <Arduino.h>

// even if the two boards aren't powered on at exactly the same moment. Keep in
// sync with CHANNEL_SYNC_TIMEOUT_MS in main_receiver.cpp.
static const unsigned long CHANNEL_BROADCAST_MS = 12000;

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
        if (readButton(JOY1_SW_PIN) || readButton(JOY2_SW_PIN)) break;
        delay(200);
    }
    stopChannelBroadcast();
}
