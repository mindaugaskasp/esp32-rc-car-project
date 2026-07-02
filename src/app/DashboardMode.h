#pragma once
#include <stdint.h>

// Drives the primary driving screen: connection-established transition, the
// no-connection safety warning, the rotating latency/loss/jitter indicator,
// and forwarding joystick input to the car.
class DashboardMode {
public:
    // Call once, right after setup() finishes bringing up ESP-NOW, so the
    // no-connection timeout has a reference point.
    void markSetupComplete();

    // Returns true if the user pressed a button to open the mode menu (caller
    // should switch to ModeSelect; no joystick data was sent this tick).
    bool update(int joystickX, int joystickY);

    // Repaints the dashboard from the latest telemetry. Also used externally
    // when ModeSelect cancels back into Dashboard.
    void show();

private:
    void showSyncWarning();
    void updateLinkStats(bool newTelemetry);

    static const unsigned long CONNECTION_MESSAGE_MS   = 3000;
    static const unsigned long NO_CONNECTION_TIMEOUT_MS = 15000;
    static const unsigned long LOSS_WINDOW_MS           = 5000;  // recompute loss% over this rolling window
    static constexpr float     MOCK_REMOTE_BATTERY_VOLTAGE = 4.10f;

    bool          _connectionEstablished   = false;
    unsigned long _connectionEstablishedAt = 0;
    bool          _dashboardShown          = false;
    unsigned long _setupCompletedAt        = 0;
    bool          _syncWarningShown        = false;
    float         _maxSpeedKmh             = 0.0f;

    unsigned long _lossWindowStart    = 0;
    uint32_t      _lossWindowSentBase = 0;
    uint32_t      _lossWindowRxBase   = 0;
    int           _lossPercent        = -1;  // -1 = not yet available
    int           _jitterMs           = -1;  // -1 = not yet available
    int           _prevRtt            = -1;
};

extern DashboardMode dashboardMode;
