#include <Arduino.h>
#include "config/Esp32Pins.h"
#include "config/ControlConfig.h"
#include "config/DebugConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "config/WifiConfig.h"
#include "drivers/controls/Controls.h"
#include "drivers/screen/DebugScreen.h"
#include "drivers/screen/Screen.h"
#include "drivers/screen/ScreenDriver.h"
#include "drivers/screen/ScreenUtils.h"
#include "drivers/screen/calibration/CalibrationFlow.h"
#include "drivers/wifi/EspNowDriver.h"
#include <esp_now.h>
#include <cmath>

void onTelemetryReceive(const uint8_t* mac, const uint8_t* incomingData, int len);

// ── Constants ────────────────────────────────────────────────────────────────
const uint8_t* carMac = RECEIVER_MAC;
static const int   JOYSTICK_CENTER_RAW             = 2048;
static const float MOCK_REMOTE_BATTERY_VOLTAGE     = 4.10f;
static const unsigned long CONNECTION_MESSAGE_MS = 3000;

// ── Top-level mode ────────────────────────────────────────────────────────────
enum class TxMode : uint8_t { Dashboard, Debug, ModeSelect, Calibration };

static TxMode currentMode = TxMode::Dashboard;
static TxMode prevMode    = TxMode::Dashboard;  // restored on ModeSelect cancel

static const int MODE_COUNT = 3;
static const char* const MODE_NAMES[MODE_COUNT] = {
    "Dashboard", "Debug Info", "Calibration"
};
static int8_t modeCursor = 0;

// ── Telemetry state ───────────────────────────────────────────────────────────
static portMUX_TYPE telemetryMux = portMUX_INITIALIZER_UNLOCKED;
static TelemetryData pendingTelemetry;
static volatile bool pendingTelemetryAvailable = false;

static TelemetryData latestTelemetry = {7.4f, 0};
static bool connectionEstablished  = false;
static bool dashboardShown         = false;
static unsigned long connectionEstablishedAt = 0;
static uint32_t lastTelemetryHash  = 0;
static int   latestLatencyMs       = -1;
static float maxSpeedKmh           = 0.0f;

// ── SW button state (opens mode selector from any mode) ───────────────────────
static bool joy1SwWas = false;
static bool joy2SwWas = false;

// ── Gesture state (ModeSelect) ────────────────────────────────────────────────
static bool msYWasUp   = false;
static bool msYWasDown = false;
static unsigned long msYUpStart = 0, msYDownStart = 0;
static const unsigned long MS_TAP_MAX_MS = 700;
// SW button state for mode selector (separate from the menu-open SW state)
static bool msSw1Was = false;
static bool msSw2Was = false;

// ── Joystick send state ───────────────────────────────────────────────────────
static int prevJoyX = -1, prevJoyY = -1;
static int lastSentX = -1, lastSentY = -1;
static unsigned long lastSendTime = 0;
static int consecX = 0, consecY = 0;
static const unsigned long SEND_REPEAT_MS = 100;

// ── Helpers ───────────────────────────────────────────────────────────────────
static uint32_t hashTelemetry(const TelemetryData& t) {
    uint32_t v = (uint32_t)(t.batteryVoltage * 100.0f + 0.5f);
    return (v << 16) | (uint32_t)(t.speedRpm & 0xFFFF);
}

static float speedKmh(int rpm) { return rpm * 0.05f; }

static void showDashboard() {
    float kmh = speedKmh(latestTelemetry.speedRpm);
    if (kmh > maxSpeedKmh) maxSpeedKmh = kmh;
    DashboardData d;
    d.carBatteryVoltage    = latestTelemetry.batteryVoltage;
    d.remoteBatteryVoltage = MOCK_REMOTE_BATTERY_VOLTAGE;
    d.speedRpm             = latestTelemetry.speedRpm;
    d.speedKmh             = kmh;
    d.maxSpeedKmh          = maxSpeedKmh;
    d.latencyMs            = latestLatencyMs;
    screen.showDashboard(d);
}

static void showModeSelector() {
    ScreenDriver* d = getScreenDriver();
    if (!d) return;
    const char* above    = (modeCursor > 0)            ? MODE_NAMES[modeCursor - 1] : nullptr;
    const char* below    = (modeCursor < MODE_COUNT-1) ? MODE_NAMES[modeCursor + 1] : nullptr;
    char selected[32];
    snprintf(selected, sizeof(selected), "> %s", MODE_NAMES[modeCursor]);
    d->clear();
    d->font(ScreenFont::Medium);
    d->text(0, 10, "SELECT MODE");
    d->hline(0, 13, ScreenDriver::W);
    d->font(ScreenFont::Small);
    if (above)    d->scrollText(26, above);
    d->scrollText(37, selected);
    if (below)    d->scrollText(48, below);
    d->hline(0, 52, ScreenDriver::W);
    d->font(ScreenFont::Tiny);
    d->text(0, 60, "Y:nav  SW2:select  SW1:back");
    d->flush();
}

// Processes incoming telemetry from the ISR queue.
// Returns true if new (changed) telemetry was available.
static bool processTelemetry() {
    TelemetryData received;
    bool available;
    portENTER_CRITICAL(&telemetryMux);
    available = pendingTelemetryAvailable;
    if (available) {
        received = pendingTelemetry;
        pendingTelemetryAvailable = false;
    }
    portEXIT_CRITICAL(&telemetryMux);

    if (!available) return false;

    uint32_t hash = hashTelemetry(received);
    if (hash == lastTelemetryHash) return false;

    latestTelemetry = received;
    lastTelemetryHash = hash;
    if (!connectionEstablished) {
        connectionEstablished   = true;
        connectionEstablishedAt = millis();
    }
    debugLogger.logf("Telemetry: %.2fV %d RPM", received.batteryVoltage, received.speedRpm);
    return true;
}

// Sends joystick data with smart change-detection and repeat logic.
static void sendJoystick(int joystickX, int joystickY) {
    unsigned long now = millis();

    if (prevJoyX < 0) {
        prevJoyX = joystickX; prevJoyY = joystickY;
        lastSentX = joystickX; lastSentY = joystickY;
        lastSendTime = now; consecX = consecY = 0;
    }

    bool movedX = abs(joystickX - prevJoyX) > JOY_DEADZONE_X;
    bool movedY = abs(joystickY - prevJoyY) > JOY_DEADZONE_Y;
    bool outsideNeutral = abs(joystickX - JOYSTICK_CENTER_RAW) > JOY_DEADZONE_X
                       || abs(joystickY - JOYSTICK_CENTER_RAW) > JOY_DEADZONE_Y;
    bool repeatSend        = outsideNeutral && (now - lastSendTime >= SEND_REPEAT_MS);
    bool changedSinceLastSend = abs(joystickX - lastSentX) > JOY_DEADZONE_X
                             || abs(joystickY - lastSentY) > JOY_DEADZONE_Y;
    bool directionChanged  = (abs(lastSentY - JOYSTICK_CENTER_RAW) > JOY_DEADZONE_Y)
                          && (abs(joystickY  - JOYSTICK_CENTER_RAW) > JOY_DEADZONE_Y)
                          && ((lastSentY < JOYSTICK_CENTER_RAW) != (joystickY < JOYSTICK_CENTER_RAW));

    if (movedX) consecX++; else consecX = 0;
    if (movedY) consecY++; else consecY = 0;
    bool quickUpdate = consecX >= JOY_CONSECUTIVE_THRESHOLD || consecY >= JOY_CONSECUTIVE_THRESHOLD;

    if (quickUpdate || repeatSend || changedSinceLastSend || directionChanged) {
        debugLogger.logJoystick(joystickX, joystickY);
        prevJoyX = joystickX; prevJoyY = joystickY;
        VehicleData data = {joystickX, joystickY, (uint32_t)millis()};
        sendData(data, carMac);
        lastSentX = joystickX; lastSentY = joystickY;
        lastSendTime = now;
        consecX = consecY = 0;
    }
}

// Returns true on the first loop where either joystick SW button is pressed.
static bool checkMenuButton() {
    bool joy1Sw = readButton(JOY1_SW_PIN);
    bool joy2Sw = readButton(JOY2_SW_PIN);
    bool pressed = (joy1Sw && !joy1SwWas) || (joy2Sw && !joy2SwWas);
    joy1SwWas = joy1Sw;
    joy2SwWas = joy2Sw;
    return pressed;
}

// ── Mode handlers ─────────────────────────────────────────────────────────────

static void handleDashboard(int joystickX, int joystickY) {
    unsigned long now = millis();

    bool newTelemetry = processTelemetry();
    if (newTelemetry) {
        if (!dashboardShown) {
            screen.showConnectionEstablished();
        } else {
            showDashboard();
        }
    }
    if (connectionEstablished && !dashboardShown
        && now - connectionEstablishedAt >= CONNECTION_MESSAGE_MS) {
        dashboardShown = true;
        showDashboard();
    }

    if (checkMenuButton()) {
        prevMode    = TxMode::Dashboard;
        modeCursor  = 0;
        currentMode = TxMode::ModeSelect;
        showModeSelector();
        return;
    }

    sendJoystick(joystickX, joystickY);
}

static void handleDebug(int joystickX, int joystickY) {
    unsigned long now = millis();

    bool newTelemetry = processTelemetry();
    if (newTelemetry) {
        debugScreen.showTelemetry(latestTelemetry.batteryVoltage, latestTelemetry.speedRpm);
    } else {
        debugScreen.showJoystickData(joystickX, joystickY);
    }

    if (checkMenuButton()) {
        prevMode    = TxMode::Debug;
        modeCursor  = 1;
        currentMode = TxMode::ModeSelect;
        showModeSelector();
        return;
    }

    sendJoystick(joystickX, joystickY);
}

static void handleModeSelect(int joystickX, int joystickY) {
    (void)joystickX;
    unsigned long now = millis();
    bool yUp   = joystickY > 3500;
    bool yDown = joystickY < 500;
    bool sw1   = readButton(JOY1_SW_PIN);
    bool sw2   = readButton(JOY2_SW_PIN);

    if (yUp   && !msYWasUp)   msYUpStart   = now;
    if (yDown && !msYWasDown) msYDownStart = now;

    if (!yUp && msYWasUp && (now - msYUpStart < MS_TAP_MAX_MS)) {
        modeCursor = (modeCursor > 0) ? modeCursor - 1 : MODE_COUNT - 1;
        showModeSelector();
    }
    if (!yDown && msYWasDown && (now - msYDownStart < MS_TAP_MAX_MS)) {
        modeCursor = (modeCursor < MODE_COUNT - 1) ? modeCursor + 1 : 0;
        showModeSelector();
    }
    if (sw2 && !msSw2Was) {  // SW2 (throttle stick) = select/confirm
        switch (modeCursor) {
            case 0: currentMode = TxMode::Dashboard; showDashboard(); break;
            case 1: currentMode = TxMode::Debug;     break;
            case 2:
                currentMode = TxMode::Calibration;
                calibrationFlow.begin();
                break;
        }
    }
    if (sw1 && !msSw1Was) {  // SW1 (steering stick) = back/cancel
        currentMode = prevMode;
        if (currentMode == TxMode::Dashboard) showDashboard();
    }

    msYWasUp = yUp; msYWasDown = yDown;
    msSw1Was = sw1; msSw2Was   = sw2;
    // No sendJoystick here — menu navigation takes over Y-axis input while menu is open.
}

static void handleCalibration(int joystickX, int joystickY) {
    VehicleData data = calibrationFlow.update(joystickX, joystickY);
    sendData(data, carMac);

    if (calibrationFlow.wantsExit()) {
        modeCursor  = 2;
        prevMode    = TxMode::Calibration;
        currentMode = TxMode::ModeSelect;
        showModeSelector();
    }
}

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);

    initButton(JOY1_SW_PIN);
    initButton(JOY2_SW_PIN);

    screen.begin();  // shows "Initializing..."

    screen.showStartup("Starting WiFi...");
    initEspNow();

    screen.showStartup("Registering peer...");
    addPeer(carMac);
    printMacAddress();

    screen.showStartup("Sending probe...");
    VehicleData probe = {JOYSTICK_CENTER_RAW, JOYSTICK_CENTER_RAW, (uint32_t)millis()};
    sendData(probe, carMac);

    esp_now_register_recv_cb(onTelemetryReceive);
    debugLogger.log("RC Remote ready");
    screen.showStartup("Waiting for car...");
}

void loop() {
    int joystickX = readInput(JOY1_X_PIN);
    int joystickY = readInput(JOY2_Y_PIN);

    switch (currentMode) {
        case TxMode::Dashboard:   handleDashboard(joystickX, joystickY);  break;
        case TxMode::Debug:       handleDebug(joystickX, joystickY);      break;
        case TxMode::ModeSelect:  handleModeSelect(joystickX, joystickY); break;
        case TxMode::Calibration: handleCalibration(joystickX, joystickY); break;
    }

    delay(20);
}

void onTelemetryReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    if (len != sizeof(TelemetryData)) return;

    TelemetryData t;
    memcpy(&t, incomingData, sizeof(t));

    uint32_t now = millis();
    int rtt = (t.echoTimestampMs > 0 && now >= t.echoTimestampMs)
              ? (int)(now - t.echoTimestampMs)
              : -1;

    portENTER_CRITICAL(&telemetryMux);
    pendingTelemetry          = t;
    pendingTelemetryAvailable = true;
    if (rtt >= 0) latestLatencyMs = rtt;
    portEXIT_CRITICAL(&telemetryMux);
}
