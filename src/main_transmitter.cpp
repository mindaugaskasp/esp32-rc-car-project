#include <Arduino.h>
#include "config/controller/Esp32Pins.h"
#include "config/ControlConfig.h"
#include "config/DebugConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "config/WifiConfig.h"
#include "drivers/controls/Controls.h"
#include "ui/Screen.h"
#include "ui/WifiPingScreen.h"
#include "ui/ModeSelectMenu.h"
#include "drivers/display/ScreenDriver.h"
#include "ui/calibration/CalibrationFlow.h"
#include "comm/ChannelScanner.h"
#include "drivers/radio/EspNowDriver.h"
#include "comm/ChannelSync.h"
#include "comm/TelemetryLink.h"
#include "comm/JoystickSender.h"
#include "app/DashboardMode.h"
#include "app/DebugMode.h"
#include "app/WifiPingMode.h"

// ── Top-level mode state machine ────────────────────────────────────────────
// Each mode's actual per-tick behavior lives in its own class (DashboardMode,
// DebugMode, WifiPingMode, CalibrationFlow, ModeSelectMenu); this file only owns
// which mode is active and how modes hand off to one another.
enum class TransmitterOperatingMode : uint8_t { Dashboard, Debug, ModeSelect, Calibration, WifiPing };

// Single source of truth for the mode menu: display label ↔ mode. The menu rows,
// the menu cursor, and the selection dispatch all derive from this table, so
// adding or reordering a selectable mode is a one-line change here.
struct MenuEntry { const char* name; TransmitterOperatingMode mode; };
static constexpr MenuEntry MENU[] = {
    {"Dashboard", TransmitterOperatingMode::Dashboard},
    {"Debug Info", TransmitterOperatingMode::Debug},
    {"Calibration", TransmitterOperatingMode::Calibration},
    {"WiFi Ping", TransmitterOperatingMode::WifiPing},
};
static constexpr int8_t MENU_COUNT = sizeof(MENU) / sizeof(MENU[0]);

static TransmitterOperatingMode currentMode = TransmitterOperatingMode::Dashboard;
static TransmitterOperatingMode prevMode = TransmitterOperatingMode::Dashboard;  // restored on ModeSelect cancel

static int8_t cursorForMode(TransmitterOperatingMode mode) {
    for (int8_t index = 0; index < MENU_COUNT; index++) {
        if (MENU[index].mode == mode) return index;
    }
    return 0;
}

// Enter a mode from the menu, running its one-time entry hook.
static void beginMode(TransmitterOperatingMode mode) {
    currentMode = mode;
    switch (mode) {
        case TransmitterOperatingMode::Dashboard: dashboardMode.show(); break;
        case TransmitterOperatingMode::Debug: debugMode.begin(); break;
        case TransmitterOperatingMode::Calibration: calibrationFlow.begin(); break;
        case TransmitterOperatingMode::WifiPing: wifiPingMode.begin(); break;
        default: break;  // ModeSelect is entered via enterModeSelect(), not here
    }
}

static void enterModeSelect(TransmitterOperatingMode from) {
    prevMode = from;
    currentMode = TransmitterOperatingMode::ModeSelect;
    modeSelectMenu.setCursor(cursorForMode(from));
    modeSelectMenu.onOpen();  // require SW release before select/back registers
    modeSelectMenu.show();
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);

    initButton(JOY1_SW_PIN);
    initButton(JOY2_SW_PIN);

    static const char* menuNames[MENU_COUNT];
    for (int8_t index = 0; index < MENU_COUNT; index++) menuNames[index] = MENU[index].name;
    modeSelectMenu.setEntries(menuNames, MENU_COUNT);

    screen.begin();  // shows "Initializing..."

    // Scan BEFORE ESP-NOW init — an active ESP-NOW session makes the scan return
    // 0 APs on every channel.
    screen.showStartup("Scanning WiFi...");
    ChannelScanResult scanResult = scanChannel();

    screen.showStartup("Starting WiFi...");
    initEspNow();

    // Advertise the chosen channel to the receiver, then switch to it.
    broadcastChannelToReceiver(scanResult);
    applyWifiChannel(scanResult.bestChannel);

    screen.showStartup("Registering peer...");
    addPeer(RECEIVER_MAC);
    printMacAddress();

    screen.showStartup("Sending probe...");
    VehicleData probe = makeNeutralCommand(static_cast<uint32_t>(millis()));
    sendData(probe, RECEIVER_MAC);

    telemetryLink.begin();
    debugLogger.log("RC Remote ready");
    screen.showStartup("Waiting for car...");
    dashboardMode.markSetupComplete();
}

// Keep-alive cadence: if a mode sent nothing this recently (idle stick, or the
// mode menu where the Y axis drives navigation instead of the car), the loop
// emits a neutral command so the receiver keeps hearing a steady frame. That
// lets the receiver treat true silence as a lost link (triggering channel
// resync) rather than mistaking an idle pause for a disconnect.
static const unsigned long HEARTBEAT_INTERVAL_MS = 300;

// Non-blocking loop cadence. The loop body runs at most once per interval and
// returns immediately in between (no delay()), so the framework's WiFi/ESP-NOW
// background tasks and any pending telemetry are serviced promptly instead of
// the loop sitting blocked. Target cycle stays in the 20-50ms window.
static const unsigned long LOOP_INTERVAL_MS = 20;

static int readJoystickX() {
    return applyAxisInvert(readInput(JOY2_X_PIN), JOY_INVERT_X, ADC_MAX_RAW);
}

static int readJoystickY() {
    return applyAxisInvert(readInput(JOY1_Y_PIN), JOY_INVERT_Y, ADC_MAX_RAW);
}

void loop() {
    static unsigned long lastTickMs = 0;
    unsigned long now = millis();
    if (now - lastTickMs < LOOP_INTERVAL_MS) return;
    lastTickMs = now;

    // Y drives menu navigation and both driving modes; read it always. X is only
    // read in the modes that steer (Dashboard, Debug, Calibration), so ModeSelect
    // and WifiPing don't pay for an unused ADC burst each tick.
    int joystickY = readJoystickY();

    switch (currentMode) {
        case TransmitterOperatingMode::Dashboard:
            if (dashboardMode.update(readJoystickX(), joystickY)) {
                enterModeSelect(TransmitterOperatingMode::Dashboard);
            }
            break;

        case TransmitterOperatingMode::Debug:
            if (debugMode.update(readJoystickX(), joystickY)) {
                enterModeSelect(TransmitterOperatingMode::Debug);
            }
            break;

        case TransmitterOperatingMode::ModeSelect: {
            ModeSelectMenu::Result result = modeSelectMenu.update(joystickY);
            if (result == ModeSelectMenu::Result::Selected) {
                modeSelectMenu.onClose();  // require SW release before re-open registers
                beginMode(MENU[modeSelectMenu.getCursor()].mode);
            } else if (result == ModeSelectMenu::Result::Cancelled) {
                modeSelectMenu.onClose();
                currentMode = prevMode;
                if (currentMode == TransmitterOperatingMode::Dashboard) dashboardMode.show();
            }
            break;
        }

        case TransmitterOperatingMode::Calibration: {
            VehicleData data = calibrationFlow.update(readJoystickX(), joystickY);
            sendData(data, RECEIVER_MAC);
            if (calibrationFlow.wantsExit()) {
                enterModeSelect(TransmitterOperatingMode::Calibration);
            }
            break;
        }

        case TransmitterOperatingMode::WifiPing:
            wifiPingMode.update();
            if (wifiPingMode.wantsExit()) {
                enterModeSelect(TransmitterOperatingMode::WifiPing);
            }
            break;
    }

    // Heartbeat — see HEARTBEAT_INTERVAL_MS. Neutral is the correct thing to send
    // while idle or navigating the menu (the car should not move), and it keeps
    // the link warm for the receiver's disconnect detection.
    if (millisSinceLastSend() >= HEARTBEAT_INTERVAL_MS) {
        VehicleData heartbeat = makeNeutralCommand(static_cast<uint32_t>(millis()));
        sendData(heartbeat, RECEIVER_MAC);
    }
}
