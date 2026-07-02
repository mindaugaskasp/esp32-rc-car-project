#include <Arduino.h>
#include "config/Esp32Pins.h"
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
enum class TxMode : uint8_t { Dashboard, Debug, ModeSelect, Calibration, WifiPing };

// Single source of truth for the mode menu: display label ↔ mode. The menu rows,
// the menu cursor, and the selection dispatch all derive from this table, so
// adding or reordering a selectable mode is a one-line change here.
struct MenuEntry { const char* name; TxMode mode; };
static constexpr MenuEntry MENU[] = {
    {"Dashboard",   TxMode::Dashboard},
    {"Debug Info",  TxMode::Debug},
    {"Calibration", TxMode::Calibration},
    {"WiFi Ping",   TxMode::WifiPing},
};
static constexpr int8_t MENU_COUNT = sizeof(MENU) / sizeof(MENU[0]);

static TxMode currentMode = TxMode::Dashboard;
static TxMode prevMode    = TxMode::Dashboard;  // restored on ModeSelect cancel

static int8_t cursorForMode(TxMode mode) {
    for (int8_t index = 0; index < MENU_COUNT; index++) {
        if (MENU[index].mode == mode) return index;
    }
    return 0;
}

// Enter a mode from the menu, running its one-time entry hook.
static void beginMode(TxMode mode) {
    currentMode = mode;
    switch (mode) {
        case TxMode::Dashboard:   dashboardMode.show();    break;
        case TxMode::Calibration: calibrationFlow.begin(); break;
        case TxMode::WifiPing:    wifiPingMode.begin();    break;
        default: break;  // Debug has no entry hook
    }
}

static void enterModeSelect(TxMode from) {
    prevMode    = from;
    currentMode = TxMode::ModeSelect;
    modeSelectMenu.setCursor(cursorForMode(from));
    modeSelectMenu.show();
}

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);

    initButton(JOY1_SW_PIN);
    initButton(JOY2_SW_PIN);

    static const char* menuNames[MENU_COUNT];
    for (int8_t index = 0; index < MENU_COUNT; index++) menuNames[index] = MENU[index].name;
    modeSelectMenu.setEntries(menuNames, MENU_COUNT);

    screen.begin();  // shows "Initializing..."

    screen.showStartup("Starting WiFi...");
    initEspNow();

    // Scan, advertise the chosen channel to the receiver, then switch to it.
    applyWifiChannel(syncChannelWithReceiver());

    screen.showStartup("Registering peer...");
    addPeer(RECEIVER_MAC);
    printMacAddress();

    screen.showStartup("Sending probe...");
    VehicleData probe = {JOYSTICK_CENTER_RAW, JOYSTICK_CENTER_RAW, (uint32_t)millis()};
    sendData(probe, RECEIVER_MAC);

    telemetryLink.begin();
    debugLogger.log("RC Remote ready");
    screen.showStartup("Waiting for car...");
    dashboardMode.markSetupComplete();
}

void loop() {
    int joystickX = readInput(JOY1_X_PIN);
    int joystickY = readInput(JOY2_Y_PIN);

    switch (currentMode) {
        case TxMode::Dashboard:
            if (dashboardMode.update(joystickX, joystickY)) {
                enterModeSelect(TxMode::Dashboard);
            }
            break;

        case TxMode::Debug:
            if (debugMode.update(joystickX, joystickY)) {
                enterModeSelect(TxMode::Debug);
            }
            break;

        case TxMode::ModeSelect: {
            ModeSelectMenu::Result result = modeSelectMenu.update(joystickY);
            if (result == ModeSelectMenu::Result::Selected) {
                beginMode(MENU[modeSelectMenu.getCursor()].mode);
            } else if (result == ModeSelectMenu::Result::Cancelled) {
                currentMode = prevMode;
                if (currentMode == TxMode::Dashboard) dashboardMode.show();
            }
            break;
        }

        case TxMode::Calibration: {
            VehicleData data = calibrationFlow.update(joystickX, joystickY);
            sendData(data, RECEIVER_MAC);
            if (calibrationFlow.wantsExit()) {
                enterModeSelect(TxMode::Calibration);
            }
            break;
        }

        case TxMode::WifiPing:
            wifiPingMode.update();
            if (wifiPingMode.wantsExit()) {
                enterModeSelect(TxMode::WifiPing);
            }
            break;
    }

    delay(20);
}
