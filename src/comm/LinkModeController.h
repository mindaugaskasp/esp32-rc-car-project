#pragma once
#include <stdint.h>
#include "comm/LinkModeLogic.h"

// Transmitter-side driver of the runtime ESP-NOW PHY switch (Standard <-> Long
// Range). Holds the desired and applied modes, stamps the desired mode onto every
// outgoing frame (via setOutgoingLinkMode), and runs the handshake each loop tick:
//
//   request -> receiver adopts and acks the new mode in telemetry (on the still-
//   shared PHY) -> this controller switches its own PHY once it sees that ack.
//
// Failure handling converges on Standard, the PHY both boards can always reach:
//   * a fresh switch that isn't confirmed by returning telemetry reverts quickly;
//   * a confirmed Long Range link that then goes silent for longer (peer reboot to
//     Standard, sustained loss) also reverts, more patiently;
//   * a request the receiver never acks while the link is otherwise alive is given
//     up, settling both boards where they already are.
//
// The pure decisions live in LinkModeLogic.h and are unit-tested. Transmitter-only.
class LinkModeController {
public:
    // Seeds state from the compile-time boot PHY (already applied by initEspNow)
    // and pushes it to the outgoing-frame stamper. Call once after telemetryLink.begin().
    void begin(bool bootLongRange);

    // User action from the Link Mode screen: request the opposite of the mode we
    // are currently on.
    void requestToggle();

    // Call every loop() tick. Reads fresh telemetry, advances the handshake, and
    // applies the revert/give-up safety rules.
    void update(unsigned long now);

    LinkPhyMode desired() const { return _desired; }
    LinkPhyMode applied() const { return _applied; }
    bool switching() const { return _desired != _applied; }
    bool confirmed() const { return _confirmed; }

private:
    // Revert an unconfirmed fresh switch this fast (a lost ack); tolerate this much
    // silence on a confirmed Long Range link before falling back to Standard.
    static constexpr unsigned long CONFIRM_TIMEOUT_MS = 1000;
    static constexpr unsigned long LINK_DEAD_REVERT_MS = 4000;
    // Give up a request the receiver never acked while the link is still alive.
    static constexpr unsigned long REQUEST_GIVE_UP_MS = 3000;
    // No telemetry for this long counts as "link not alive" for the give-up rule.
    static constexpr unsigned long LINK_ALIVE_WINDOW_MS = 500;

    void setDesired(LinkPhyMode mode, unsigned long now);

    LinkPhyMode _desired = LinkPhyMode::Standard;
    LinkPhyMode _applied = LinkPhyMode::Standard;
    bool _confirmed = true; // applied PHY is confirmed working by returning telemetry
    unsigned long _switchAt = 0; // when _applied last changed
    unsigned long _requestAt = 0; // when _desired last changed
    unsigned long _lastTelemetryAt = 0;
    uint32_t _lastSeenTelemetryCount = 0;
};

extern LinkModeController linkModeController;
