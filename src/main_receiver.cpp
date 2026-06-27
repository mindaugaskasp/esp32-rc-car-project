#include <Arduino.h>
#include "config/DebugConfig.h"
#include "config/WifiConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/servo/ServoDriver.h"
#include "drivers/esc/EscDriver.h"
#include "drivers/wifi/EspNowDriver.h"
#include <esp_now.h>

static unsigned long lastPacketTime = 0;
static bool escResetDueToLoss = false;
static const unsigned long PACKET_LOSS_TIMEOUT_MS = 500;

struct PendingPacket {
    uint8_t mac[6];
    uint8_t data[sizeof(VehicleData)];
    int len;
};

static portMUX_TYPE pendingPacketMux = portMUX_INITIALIZER_UNLOCKED;
static PendingPacket pendingPacket;
static volatile bool pendingPacketAvailable = false;

void onDataReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
    unsigned long receivedAt = millis();
    size_t copyLen = 0;
    if (len > 0) {
        copyLen = len < static_cast<int>(sizeof(pendingPacket.data))
            ? len
            : sizeof(pendingPacket.data);
    }

    portENTER_CRITICAL(&pendingPacketMux);
    memcpy(pendingPacket.mac, mac, sizeof(pendingPacket.mac));
    if (copyLen > 0) {
        memcpy(pendingPacket.data, incomingData, copyLen);
    }
    pendingPacket.len = len;
    pendingPacketAvailable = true;
    lastPacketTime = receivedAt;
    escResetDueToLoss = false;
    portEXIT_CRITICAL(&pendingPacketMux);
}

static void handleVehiclePacket(const uint8_t *mac, const VehicleData &data) {
    setServoAngle(data.servoPos);
    updateEscSpeed(data.escSpeed);
    
    TelemetryData telemetry;
    telemetry.batteryVoltage = 7.4; // Replace with actual voltage function
    telemetry.speedRpm = 50;        // Replace with actual speed function
    
    debugLogger.logf("Received data: servo=%d esc=%d", data.servoPos, data.escSpeed);

    // Send back to the sender's MAC address
    esp_err_t result = esp_now_send(mac, (uint8_t *)&telemetry, sizeof(telemetry));
    if (result == ESP_OK) {
        debugLogger.log("Telemetry packet sent back to sender");
    } else {
        debugLogger.logf("Telemetry send failed: %d", result);
    }
}

static void processPendingPacket() {
    PendingPacket packet;
    bool packetAvailable;

    portENTER_CRITICAL(&pendingPacketMux);
    packetAvailable = pendingPacketAvailable;
    if (packetAvailable) {
        memcpy(&packet, &pendingPacket, sizeof(packet));
        pendingPacketAvailable = false;
    }
    portEXIT_CRITICAL(&pendingPacketMux);

    if (!packetAvailable) {
        return;
    }

    char peerMac[18];
    snprintf(peerMac, sizeof(peerMac), "%02X:%02X:%02X:%02X:%02X:%02X",
             packet.mac[0], packet.mac[1], packet.mac[2], packet.mac[3], packet.mac[4], packet.mac[5]);
    debugLogger.logf("Packet received from: %s, len=%d", peerMac, packet.len);

    const char* packetType = (packet.len == sizeof(VehicleData)) ? "VehicleData" : "Unknown";
    debugLogger.logf("Packet type: %s", packetType);

    if (packet.len != sizeof(VehicleData)) {
        return;
    }

    VehicleData data;
    memcpy(&data, packet.data, sizeof(data));
    handleVehiclePacket(packet.mac, data);
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);  // Wait for serial monitor to connect
    debugLogger.enableScreenLogging(DEBUG_SCREEN);
    debugLogger.log("RC Car Starting...");
    printMacAddress();

    initServo();
    initEsc();
    debugLogger.log("Servo and ESC initialized");
    
    initEspNow();
    addPeer(TRANSMITTER_MAC);
    initEspNowReceiver();
    debugLogger.log("Receiver ready and waiting for ESP-NOW packets");
}

void loop() {
    processPendingPacket();

    unsigned long now = millis();
    bool shouldResetEsc = false;

    portENTER_CRITICAL(&pendingPacketMux);
    if (!escResetDueToLoss && lastPacketTime > 0 && now - lastPacketTime > PACKET_LOSS_TIMEOUT_MS) {
        escResetDueToLoss = true;
        shouldResetEsc = true;
    }
    portEXIT_CRITICAL(&pendingPacketMux);

    if (shouldResetEsc) {
        setEscNeutral();
        debugLogger.log("No ESP-NOW packet received recently; ESC reset to neutral");
    }
}
