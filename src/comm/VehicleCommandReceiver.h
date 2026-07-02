#pragma once
#include <Arduino.h>
#include "comm/DataTypes.h"

// Owns the receiver's ESP-NOW inbound command stream: ISR-safe receipt,
// dispatch to servo/ESC, telemetry echo-back, and the packet-loss watchdog
// that returns the ESC to neutral if the transmitter goes quiet.
class VehicleCommandReceiver {
public:
    void begin(); // registers the ESP-NOW receive callback
    void update(); // call every loop(): dispatches pending packets, runs the watchdog

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

    void dispatch(const uint8_t* mac, const VehicleData& data);

    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    PendingPacket _pending = {};
    volatile bool _pendingAvailable = false;

    unsigned long _lastPacketTime = 0;
    bool _escResetDueToLoss = false;
};

extern VehicleCommandReceiver vehicleCommandReceiver;
