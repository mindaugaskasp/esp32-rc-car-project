#include "VehicleCommandReceiver.h"
#include "drivers/servo/ServoDriver.h"
#include "drivers/esc/EscDriver.h"
#include "drivers/hall/HallSensorDriver.h"
#include "drivers/debug/DebugLogger.h"
#include <esp_now.h>

VehicleCommandReceiver vehicleCommandReceiver;

static void onVehicleDataReceiveTrampoline(const uint8_t* mac, const uint8_t* incomingData, int len) {
    vehicleCommandReceiver.handleReceive(mac, incomingData, len);
}

void VehicleCommandReceiver::begin() {
    esp_now_register_recv_cb(onVehicleDataReceiveTrampoline);
}

void VehicleCommandReceiver::handleReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    unsigned long receivedAt = millis();
    size_t copyLength = 0;
    if (len > 0) {
        copyLength = len < static_cast<int>(sizeof(_pending.data))
            ? len
            : sizeof(_pending.data);
    }

    portENTER_CRITICAL(&_mux);
    memcpy(_pending.mac, mac, sizeof(_pending.mac));
    if (copyLength > 0) {
        memcpy(_pending.data, incomingData, copyLength);
    }
    _pending.len = len;
    _pendingAvailable = true;
    _lastPacketTime = receivedAt;
    _escResetDueToLoss = false;
    portEXIT_CRITICAL(&_mux);
}

void VehicleCommandReceiver::dispatch(const uint8_t* mac, const VehicleData& data) {
    setServoAngle(data.servoPosition);
    updateEscSpeed(data.escSpeed);

    TelemetryData telemetry;
    telemetry.batteryVoltage = 7.4f; // TODO: replace with ADC voltage divider reading
    telemetry.speedRpm = getMotorRpm();
    telemetry.echoTimestampMs = data.txTimestampMs;

    debugLogger.logf("Received data: servo=%d esc=%d", data.servoPosition, data.escSpeed);

    esp_err_t result = esp_now_send(mac, reinterpret_cast<const uint8_t*>(&telemetry), sizeof(telemetry));
    if (result == ESP_OK) {
        debugLogger.log("Telemetry packet sent back to sender");
    } else {
        debugLogger.logf("Telemetry send failed: %d", result);
    }
}

void VehicleCommandReceiver::update() {
    PendingPacket packet;
    bool packetAvailable;

    portENTER_CRITICAL(&_mux);
    packetAvailable = _pendingAvailable;
    if (packetAvailable) {
        memcpy(&packet, &_pending, sizeof(packet));
        _pendingAvailable = false;
    }
    portEXIT_CRITICAL(&_mux);

    if (packetAvailable) {
        char peerMac[18];
        snprintf(peerMac, sizeof(peerMac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 packet.mac[0], packet.mac[1], packet.mac[2], packet.mac[3], packet.mac[4], packet.mac[5]);
        debugLogger.logf("Packet received from: %s, len=%d", peerMac, packet.len);

        const char* packetType = (packet.len == sizeof(VehicleData)) ? "VehicleData" : "Unknown";
        debugLogger.logf("Packet type: %s", packetType);

        if (packet.len == sizeof(VehicleData)) {
            VehicleData data;
            memcpy(&data, packet.data, sizeof(data));
            dispatch(packet.mac, data);
        }
    }

    unsigned long now = millis();
    bool shouldResetEsc = false;

    portENTER_CRITICAL(&_mux);
    if (!_escResetDueToLoss && _lastPacketTime > 0 && now - _lastPacketTime > PACKET_LOSS_TIMEOUT_MS) {
        _escResetDueToLoss = true;
        shouldResetEsc = true;
    }
    portEXIT_CRITICAL(&_mux);

    if (shouldResetEsc) {
        setEscNeutral();
        debugLogger.log("No ESP-NOW packet received recently; ESC reset to neutral");
    }
}
