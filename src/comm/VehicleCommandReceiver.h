#pragma once
#include <Arduino.h>
#include "comm/DataTypes.h"
#include "comm/LinkModeLogic.h"

// Owns the receiver's ESP-NOW inbound command stream: ISR-safe receipt,
// dispatch to servo/ESC, telemetry echo-back, and the packet-loss watchdog
// that returns the ESC to neutral if the transmitter goes quiet.
class VehicleCommandReceiver {
public:
    void begin(); // registers the ESP-NOW receive callback
    void update(); // call every loop(): dispatches pending packets, runs the watchdog

    // True once a real command has arrived and the stream is still inside the
    // watchdog window — i.e. the transmitter link is up right now.
    bool isLinkAlive() const;

    // ESP-NOW callback entry point. Public only because the C callback API
    // can't reach a private member — not part of the intended call surface.
    void handleReceive(const uint8_t* mac, const uint8_t* incomingData, int len);

private:
    struct PendingPacket {
        uint8_t mac[6];
        uint8_t data[sizeof(VehicleData)];
        int len;
    };

    static const unsigned long PACKET_LOSS_TIMEOUT_MS = 500;

    // Channel resync: if the link stays down this long, the transmitter has
    // likely restarted and may have moved to a new WiFi channel. We then hop to
    // the advertisement channel to listen for a fresh channel advertisement,
    // alternating with the operational channel so we also catch a transmitter
    // that resumes on the same channel.
    static const unsigned long RESYNC_AFTER_LOSS_MS = 3000;
    static const unsigned long RESYNC_LISTEN_MS = 1500; // time spent on the advertisement channel per cycle
    static const unsigned long RESYNC_SERVE_MS = 1500;  // time back on the operational channel per cycle

    void dispatch(const uint8_t* mac, const VehicleData& data);
    void updateResync(unsigned long now);

    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    PendingPacket _pending = {};
    volatile bool _pendingAvailable = false;

    unsigned long _lastPacketTime = 0;
    bool _escResetDueToLoss = false;
    int _armingCount = 0; // failsafe arming counter (see comm/ArmingLogic.h)
    // Set by the callback when a command arrives after a silence longer than the
    // watchdog timeout — covers the race where the packet lands before update()
    // has noticed the dropout, so re-arming is never skipped.
    volatile bool _rearmAfterGapPending = false;
    bool _linkTwitchDone = false; // one-shot "link established" servo twitch on first real command
    // _lastPacketTime is seeded in begin(), so it alone can't distinguish "never
    // heard the transmitter" from "link up"; this flag makes that distinction.
    volatile bool _commandEverReceived = false;

    // Currently applied PHY. Boot default is Standard (matches initEspNow); adopts
    // the transmitter's desired PHY via the handshake in dispatch(), and reverts to
    // Standard whenever the link is lost long enough to enter resync — so a stranded
    // Long Range receiver drops back to the PHY the transmitter always reboots into.
    LinkPhyMode _appliedLinkMode = LinkPhyMode::Standard;

    uint8_t _operationalChannel = 0;      // the channel we serve commands on
    bool _listeningForResync = false;     // true while hopped to the advertisement channel
    unsigned long _resyncPhaseStart = 0;  // millis() of the current listen/serve phase
    volatile bool _resyncChannelPending = false; // set by the callback when an advertisement arrives
    volatile uint8_t _resyncChannel = 0;
};

extern VehicleCommandReceiver vehicleCommandReceiver;
