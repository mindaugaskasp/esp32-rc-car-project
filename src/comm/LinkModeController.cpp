#include "LinkModeController.h"
#include "comm/TelemetryLink.h"
#include "drivers/radio/EspNowDriver.h"
#include "drivers/debug/DebugLogger.h"
#include <Arduino.h>

LinkModeController linkModeController;

static const char* modeName(LinkPhyMode mode) {
    return mode == LinkPhyMode::LongRange ? "Long Range" : "Standard";
}

void LinkModeController::begin(bool bootLongRange) {
    _applied = bootLongRange ? LinkPhyMode::LongRange : LinkPhyMode::Standard;
    _desired = _applied;
    _confirmed = true;
    unsigned long now = millis();
    _switchAt = now;
    _requestAt = now;
    _lastTelemetryAt = now;
    _lastSeenTelemetryCount = telemetryLink.getReceivedCount();
    setOutgoingLinkMode(linkModeToWire(_desired));
}

void LinkModeController::setDesired(LinkPhyMode mode, unsigned long now) {
    _desired = mode;
    _requestAt = now;
    setOutgoingLinkMode(linkModeToWire(_desired));
}

void LinkModeController::requestToggle() {
    setDesired(toggledLinkMode(_applied), millis());
    debugLogger.logf("[LINKMODE] Requested %s", modeName(_desired));
}

void LinkModeController::update(unsigned long now) {
    uint32_t telemetryCount = telemetryLink.getReceivedCount();
    if (telemetryCount != _lastSeenTelemetryCount) {
        _lastSeenTelemetryCount = telemetryCount;
        _lastTelemetryAt = now;

        LinkPhyMode receiverApplied = linkModeFromWire(telemetryLink.getRemoteLinkModeRaw());
        if (txShouldAdopt(receiverApplied, _desired, _applied)) {
            // Receiver has moved to the desired PHY and told us so on the shared PHY;
            // now it is safe for us to follow it there.
            applyLinkPhyMode(_desired == LinkPhyMode::LongRange);
            _applied = _desired;
            _switchAt = now;
            _confirmed = false;
            debugLogger.logf("[LINKMODE] Switched PHY to %s", modeName(_applied));
        } else if (receiverApplied == _applied) {
            _confirmed = true; // telemetry consistent with our PHY == the link works here
        }
    }

    unsigned long silentMs = now - _lastTelemetryAt;

    if (shouldRevertToStandard(_applied, _confirmed, silentMs, CONFIRM_TIMEOUT_MS, LINK_DEAD_REVERT_MS)) {
        applyLinkPhyMode(false);
        _applied = LinkPhyMode::Standard;
        _confirmed = true;
        _switchAt = now;
        setDesired(LinkPhyMode::Standard, now);
        debugLogger.log("[LINKMODE] Link silent — reverted to Standard");
        return;
    }

    bool linkAlive = silentMs < LINK_ALIVE_WINDOW_MS;
    if (txShouldGiveUpRequest(_desired, _applied, linkAlive, now - _requestAt, REQUEST_GIVE_UP_MS)) {
        setDesired(_applied, now);
        debugLogger.logf("[LINKMODE] Request unacked — staying on %s", modeName(_applied));
    }
}
