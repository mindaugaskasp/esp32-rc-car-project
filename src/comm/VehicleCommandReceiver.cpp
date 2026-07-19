#include "VehicleCommandReceiver.h"
#include "comm/ArmingLogic.h"
#include "drivers/radio/EspNowDriver.h"
#include "comm/ChannelAdvertiser.h"
#include "comm/ChannelScanner.h"
#include "config/DebugConfig.h"
#include "config/WifiConfig.h"
#include "drivers/servo/ServoDriver.h"
#include "drivers/esc/EscDriver.h"
#include "drivers/hall/HallSensorDriver.h"
#include "drivers/battery/BatteryMonitorDriver.h"
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

    // Only a well-formed command counts as proof of a live link. Wrong-size
    // packets are still buffered (so packet tracing can report them) but must
    // not feed the watchdog — otherwise a stream of undispatched packets could
    // hold the ESC at its last speed forever.
    bool validCommandLength = (len == static_cast<int>(sizeof(VehicleData)));

    portENTER_CRITICAL(&_mux);
    memcpy(_pending.mac, mac, sizeof(_pending.mac));
    if (copyLength > 0) {
        memcpy(_pending.data, incomingData, copyLength);
    }
    _pending.len = len;
    _pendingAvailable = true;
    if (validCommandLength) {
        // A command landing after watchdog-length silence must force re-arming
        // even if update() never got to run during the gap (e.g. loop() was
        // blocked); clearing _escResetDueToLoss below would otherwise hide the
        // dropout from the watchdog entirely.
        if (receivedAt - _lastPacketTime > PACKET_LOSS_TIMEOUT_MS) {
            _rearmAfterGapPending = true;
        }
        _lastPacketTime = receivedAt;
        _escResetDueToLoss = false;
    }
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

    // Failsafe arming (see comm/ArmingLogic.h): the ESC is held at neutral until
    // a short run of near-neutral throttle commands has arrived, re-armed from
    // zero after every packet-loss timeout. A stray in-range packet can't spin
    // the motor, and a link that drops at full throttle can't slam back to full
    // throttle on reconnect — the stick must return home first. Steering stays
    // live throughout; only propulsion is gated.
    _armingCount = nextArmingCount(_armingCount, data.escSpeed);
    if (isArmed(_armingCount)) {
        updateEscSpeed(data.escSpeed);
    } else {
        setEscNeutral();
    }

    // Link-mode handshake: the command carries the transmitter's desired PHY. Adopt
    // it if it differs, but ack the new mode in this telemetry frame and send that
    // frame FIRST, while both boards are still on the shared PHY — only then switch.
    // That guarantees the transmitter hears "I'm on the new PHY" before either board
    // leaves the current one. See comm/LinkModeLogic.h.
    LinkPhyMode transmitterDesired = linkModeFromWire(data.linkMode);
    bool adoptLinkMode = rxShouldAdopt(transmitterDesired, _appliedLinkMode);
    LinkPhyMode nextLinkMode = adoptLinkMode ? transmitterDesired : _appliedLinkMode;

    TelemetryData telemetry;
    telemetry.batteryVoltage = getBatteryVoltage();
    telemetry.speedRpm = getMotorRpm();
    telemetry.echoTimestampMs = data.txTimestampMs;
    telemetry.linkMode = linkModeToWire(nextLinkMode);

    if (DEBUG_PACKET_TRACE) {
        debugLogger.logf("Received data: servo=%d esc=%d", data.servoPosition, data.escSpeed);
    }

    esp_err_t result = esp_now_send(mac, reinterpret_cast<const uint8_t*>(&telemetry), sizeof(telemetry));
    if (result != ESP_OK) {
        debugLogger.logf("Telemetry send failed: %d", result);
    } else if (DEBUG_PACKET_TRACE) {
        debugLogger.log("Telemetry packet sent back to sender");
    }

    if (adoptLinkMode) {
        applyLinkPhyMode(nextLinkMode == LinkPhyMode::LongRange);
        _appliedLinkMode = nextLinkMode;
        debugLogger.logf("[LINKMODE] Adopted %s from transmitter",
                         nextLinkMode == LinkPhyMode::LongRange ? "Long Range" : "Standard");
    }
}

void VehicleCommandReceiver::update() {
    PendingPacket packet;
    bool packetAvailable;
    bool rearmAfterGap;

    portENTER_CRITICAL(&_mux);
    packetAvailable = _pendingAvailable;
    if (packetAvailable) {
        memcpy(&packet, &_pending, sizeof(packet));
        _pendingAvailable = false;
    }
    rearmAfterGap = _rearmAfterGapPending;
    _rearmAfterGapPending = false;
    portEXIT_CRITICAL(&_mux);

    // Consume before dispatch so the packet that ended the silence is the first
    // frame of the fresh arming sequence, not a continuation of the old one.
    if (rearmAfterGap) {
        _armingCount = 0;
        debugLogger.log("Command gap exceeded watchdog timeout; re-arming required");
    }

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
        _armingCount = 0; // require re-arming before propulsion resumes
        setEscNeutral();
        // Center the steering too: coasting with the wheels at the last commanded
        // lock would swerve the car for as long as the link stays down.
        setServoAngle(STEERING_CENTER_RAW);
        debugLogger.log("No ESP-NOW packet received recently; ESC neutral, steering centered");
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

    // Link lost long enough that the transmitter has likely reset. A reset
    // transmitter comes up on the boot PHY (Standard), so a receiver still on Long
    // Range would never hear it. Drop back to Standard once here so the two always
    // re-converge on the PHY the transmitter reboots into.
    if (_appliedLinkMode != LinkPhyMode::Standard) {
        applyLinkPhyMode(false);
        _appliedLinkMode = LinkPhyMode::Standard;
        debugLogger.log("[LINKMODE] Link lost — reverted PHY to Standard");
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
