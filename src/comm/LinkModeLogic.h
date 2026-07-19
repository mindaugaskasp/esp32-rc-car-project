// LinkModeLogic.h — pure decision logic for the runtime ESP-NOW PHY switch
// (Standard <-> Long Range). No Arduino or hardware dependency so it can be
// unit-tested natively (see test/test_link_mode_logic).
//
// Coordination model: the two boards must always agree on the PHY, or they go
// deaf to each other. Standard is the universal fallback both boards can always
// reach, so every failure path converges back to Standard. LongRange is only
// ever entered by a confirmed handshake; any silence during or after a switch
// drops the board back to Standard on its own, so the pair re-converges there.
#pragma once
#include <stdint.h>

enum class LinkPhyMode : uint8_t { Standard = 0, LongRange = 1 };

inline uint8_t linkModeToWire(LinkPhyMode mode) {
    return static_cast<uint8_t>(mode);
}

inline LinkPhyMode linkModeFromWire(uint8_t wire) {
    return wire == static_cast<uint8_t>(LinkPhyMode::LongRange)
        ? LinkPhyMode::LongRange
        : LinkPhyMode::Standard;
}

inline LinkPhyMode toggledLinkMode(LinkPhyMode mode) {
    return mode == LinkPhyMode::Standard ? LinkPhyMode::LongRange : LinkPhyMode::Standard;
}

// Receiver: a command carries the transmitter's desired PHY. The receiver adopts
// it whenever it differs from what the receiver is currently on. It acks the new
// mode in telemetry (on the current, still-shared PHY) *before* switching, so the
// transmitter learns of the switch on a PHY both boards can still hear.
inline bool rxShouldAdopt(LinkPhyMode transmitterDesired, LinkPhyMode receiverApplied) {
    return transmitterDesired != receiverApplied;
}

// Transmitter: switch its own PHY only once telemetry confirms the receiver has
// already moved to the desired mode. This ordering guarantees the ack is sent on
// the shared PHY before either board leaves it.
inline bool txShouldAdopt(LinkPhyMode receiverApplied, LinkPhyMode transmitterDesired,
                          LinkPhyMode transmitterApplied) {
    return transmitterApplied != transmitterDesired && receiverApplied == transmitterDesired;
}

// Safety net (both boards): if we are on a non-Standard PHY and the link has gone
// silent for the timeout, revert to Standard. The timeout is short while a fresh
// switch is still unconfirmed (fast recovery from a lost ack) and long once the
// Long Range link is confirmed working (so ordinary out-of-range gaps don't force
// a counter-productive downgrade — only a sustained loss, e.g. a peer reboot to
// Standard, does).
inline bool shouldRevertToStandard(LinkPhyMode applied, bool confirmed, unsigned long silentMs,
                                   unsigned long unconfirmedTimeoutMs, unsigned long confirmedTimeoutMs) {
    if (applied == LinkPhyMode::Standard) return false;
    return silentMs >= (confirmed ? confirmedTimeoutMs : unconfirmedTimeoutMs);
}

// Transmitter: give up a request the receiver never acked while the link is still
// alive (acks kept getting lost). Resetting the desired mode back to the applied
// mode stops the endless requesting and settles both boards where they already are.
inline bool txShouldGiveUpRequest(LinkPhyMode transmitterDesired, LinkPhyMode transmitterApplied,
                                  bool linkAlive, unsigned long sinceRequestMs, unsigned long giveUpMs) {
    return linkAlive && transmitterDesired != transmitterApplied && sinceRequestMs >= giveUpMs;
}
