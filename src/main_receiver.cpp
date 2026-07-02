#include <Arduino.h>
#include "config/DebugConfig.h"
#include "config/WifiConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/servo/ServoDriver.h"
#include "drivers/esc/EscDriver.h"
#include "drivers/hall/HallSensorDriver.h"
#include "drivers/radio/EspNowDriver.h"
#include "comm/ChannelScanner.h"
#include "comm/ChannelAdvertiser.h"
#include "comm/VehicleCommandReceiver.h"

// Worst-case transmitter startup is a multi-second channel scan followed by an
// 8s broadcast window (see CHANNEL_BROADCAST_MS in main_transmitter.cpp). This
// timeout must comfortably exceed that so the two boots have real overlap even
// if they aren't powered on at exactly the same moment.
static const unsigned long CHANNEL_SYNC_TIMEOUT_MS = 20000;
static const uint8_t CHANNEL_SYNC_FALLBACK = 6;

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500); // Wait for serial monitor to connect
    debugLogger.log("RC Car Starting...");
    printMacAddress();

    initServo();
    initEsc();
    initHallSensor();
    debugLogger.log("Servo, ESC, and Hall sensor initialized");

    initEspNow();

    // Wait for a channel advertisement from our transmitter, fall back to CHANNEL_SYNC_FALLBACK
    uint8_t channel = receiveChannelAdvertisement(TRANSMITTER_MAC, CHANNEL_SYNC_TIMEOUT_MS, CHANNEL_SYNC_FALLBACK);
    applyWifiChannel(channel);

    addPeer(TRANSMITTER_MAC);
    vehicleCommandReceiver.begin();
    debugLogger.log("Receiver ready and waiting for ESP-NOW packets");
}

void loop() {
    updateHallSensor();
    vehicleCommandReceiver.update();
}
