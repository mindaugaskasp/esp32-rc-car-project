#include "VehicleCommandReceiver.h"
#include "drivers/radio/EspNowDriver.h"
#include "comm/ChannelAdvertiser.h"
#include "comm/ChannelScanner.h"
#include "config/DebugConfig.h"
#include "config/WifiConfig.h"
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
    _operationalChannel = getSelectedChannel();
    _lastPacketTime = millis(); // measure link loss from begin(), even before the first command
    esp_now_register_recv_cb(onVehicleDataReceiveTrampoline);
}

void VehicleCommandReceiver::handleReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    // A channel advertisement means the transmitter (re)started and is announcing
    // its channel. Stash it for update() to adopt; it is not a driving command, so
    // it must not refresh the packet-loss timer.
    uint8_t advertisedChannel = 0;
    if (tryParseChannelAdvertisement(mac, incomingData, len, TRANSMITTER_MAC, &advertisedChannel)) {
        portENTER_CRITICAL(&_mux);
        _resyncChannel = advertisedChannel;
        _resyncChannelPending = true;
        portEXIT_CRITICAL(&_mux);
        return;
    }

    // Safety: only the paired transmitter may command the vehicle. Dropping any
    // other sender here means stray ESP-NOW traffic can never drive the ESC or
    // servo, and it never refreshes the packet-loss timer — so an absent
    // transmitter leaves the watchdog to hold the ESC at neutral.
    if (memcmp(mac, TRANSMITTER_MAC, MAC_ADDRESS_LENGTH) != 0) return;

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
    // First real command from the paired transmitter = a confirmed bidirectional
    // link (we received a command and are about to echo telemetry back), mirroring
    // when the remote's screen shows "connected". Blocking twitch is fine as a
    // one-time connection event; it runs before the car is armed to drive.
    if (!_linkTwitchDone) {
        _linkTwitchDone = true;
        twitchServo();
    }

    setServoAngle(data.servoPosition);

    // Failsafe arming: the ESC is held at neutral until a short run of valid
    // commands has arrived, and re-armed from zero after every packet-loss
    // timeout (see update()). This guarantees the motor cannot spin from a
    // single stray in-range packet or on the first frame after a dropout —
    // steering stays live throughout, only propulsion is gated.
    if (_consecutiveValidCommands < ARM_COMMAND_THRESHOLD) {
        _consecutiveValidCommands++;
        setEscNeutral();
    } else {
        updateEscSpeed(data.escSpeed);
    }

    TelemetryData telemetry;
    telemetry.batteryVoltage = 7.4f; // TODO: replace with ADC voltage divider reading
    telemetry.speedRpm = getMotorRpm();
    telemetry.echoTimestampMs = data.txTimestampMs;

    if (DEBUG_PACKET_TRACE) {
        debugLogger.logf("Received data: servo=%d esc=%d", data.servoPosition, data.escSpeed);
    }

    esp_err_t result = esp_now_send(mac, reinterpret_cast<const uint8_t*>(&telemetry), sizeof(telemetry));
    if (result != ESP_OK) {
        debugLogger.logf("Telemetry send failed: %d", result);
    } else if (DEBUG_PACKET_TRACE) {
        debugLogger.log("Telemetry packet sent back to sender");
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
        if (DEBUG_PACKET_TRACE) {
            char peerMac[MAC_STRING_BUFFER_SIZE];
            formatMac(peerMac, packet.mac);
            debugLogger.logf("Packet received from: %s, len=%d", peerMac, packet.len);

            const char* packetType = (packet.len == sizeof(VehicleData)) ? "VehicleData" : "Unknown";
            debugLogger.logf("Packet type: %s", packetType);
        }

        if (packet.len == sizeof(VehicleData)) {
            VehicleData data;
            memcpy(&data, packet.data, sizeof(data));
            dispatch(packet.mac, data);
        }
    }

    unsigned long now = millis();

    // Adopt a channel learned from a fresh advertisement (transmitter restarted,
    // possibly on a new channel). Peer channel is 0, so it follows the WiFi channel.
    bool adoptChannel = false;
    uint8_t newChannel = 0;
    portENTER_CRITICAL(&_mux);
    if (_resyncChannelPending) {
        adoptChannel = true;
        newChannel = _resyncChannel;
        _resyncChannelPending = false;
    }
    portEXIT_CRITICAL(&_mux);

    if (adoptChannel) {
        debugLogger.logf("[RESYNC] Advertisement heard: channel %d (was %d)", newChannel, _operationalChannel);
        _operationalChannel = newChannel;
        applyWifiChannel(newChannel);
        _listeningForResync = false;
        portENTER_CRITICAL(&_mux);
        _lastPacketTime = now; // give the resynced link time before re-triggering resync
        portEXIT_CRITICAL(&_mux);
        return; // ESC stays neutral until a real command arrives on the new channel
    }

    bool shouldResetEsc = false;

    portENTER_CRITICAL(&_mux);
    if (!_escResetDueToLoss && _lastPacketTime > 0 && now - _lastPacketTime > PACKET_LOSS_TIMEOUT_MS) {
        _escResetDueToLoss = true;
        shouldResetEsc = true;
    }
    portEXIT_CRITICAL(&_mux);

    if (shouldResetEsc) {
        _consecutiveValidCommands = 0; // require re-arming before propulsion resumes
        setEscNeutral();
        debugLogger.log("No ESP-NOW packet received recently; ESC reset to neutral");
    }

    updateResync(now);
}

void VehicleCommandReceiver::updateResync(unsigned long now) {
    unsigned long lastPacket;
    portENTER_CRITICAL(&_mux);
    lastPacket = _lastPacketTime;
    portEXIT_CRITICAL(&_mux);

    bool linkLost = (now - lastPacket) > RESYNC_AFTER_LOSS_MS;

    if (!linkLost) {
        // Link healthy — make sure we're back on the operational channel if a
        // previous resync cycle left us listening.
        if (_listeningForResync) {
            applyWifiChannel(_operationalChannel);
            _listeningForResync = false;
        }
        return;
    }

    // Link lost long enough: alternate between listening on the advertisement
    // channel (to catch a transmitter that moved channels) and serving on the
    // operational channel (to catch one that resumes on the same channel).
    if (_listeningForResync) {
        if (now - _resyncPhaseStart >= RESYNC_LISTEN_MS) {
            applyWifiChannel(_operationalChannel);
            _listeningForResync = false;
            _resyncPhaseStart = now;
        }
    } else {
        if (now - _resyncPhaseStart >= RESYNC_SERVE_MS) {
            applyWifiChannel(ADVERTISEMENT_CHANNEL);
            _listeningForResync = true;
            _resyncPhaseStart = now;
            debugLogger.logf("[RESYNC] Link down — listening on ch %d for advertisement", ADVERTISEMENT_CHANNEL);
        }
    }
}
