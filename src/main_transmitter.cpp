#include <Arduino.h>
#include "config/Esp32Pins.h"
#include "config/ControlConfig.h"
#include "config/DebugConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "config/WifiConfig.h"
#include "drivers/controls/Controls.h"
#include "drivers/screen/DebugScreen.h"
#include "drivers/screen/Screen.h"
#if CALIBRATION_MODE
#  include "drivers/screen/calibration/CalibrationFlow.h"
#endif
#include "drivers/wifi/EspNowDriver.h"
#include <esp_now.h>
#include <cmath>

void onTelemetryReceive(const uint8_t * mac, const uint8_t *incomingData, int len);

const uint8_t* carMac = RECEIVER_MAC;
static const int JOYSTICK_CENTER_RAW = 2048;
static const float MOCK_REMOTE_BATTERY_VOLTAGE = 4.10f;
static const unsigned long CONNECTION_MESSAGE_DURATION_MS = 3000;

static portMUX_TYPE telemetryMux = portMUX_INITIALIZER_UNLOCKED;
static TelemetryData pendingTelemetry;
static volatile bool pendingTelemetryAvailable = false;
static TelemetryData latestTelemetry = {7.4f, 0};
static bool connectionEstablished = false;
static bool dashboardShown = false;
static unsigned long connectionEstablishedAt = 0;
static uint32_t lastTelemetryHash = 0;

static uint32_t hashTelemetry(const TelemetryData &telemetry) {
    uint32_t voltageHash = (uint32_t)(telemetry.batteryVoltage * 100.0f + 0.5f);
    return (voltageHash << 16) | (uint32_t)(telemetry.speedRpm & 0xFFFF);
}

static float mockSpeedKmh(int speedRpm) {
    return speedRpm * 0.05f;
}

static void showDashboard() {
    screen.showDashboard(
        latestTelemetry.batteryVoltage,
        MOCK_REMOTE_BATTERY_VOLTAGE,
        latestTelemetry.speedRpm,
        mockSpeedKmh(latestTelemetry.speedRpm)
    );
}

static void processTelemetryScreenUpdates() {
    TelemetryData telemetry;
    bool telemetryAvailable;

    portENTER_CRITICAL(&telemetryMux);
    telemetryAvailable = pendingTelemetryAvailable;
    if (telemetryAvailable) {
        telemetry = pendingTelemetry;
        pendingTelemetryAvailable = false;
    }
    portEXIT_CRITICAL(&telemetryMux);

    if (telemetryAvailable) {
        uint32_t newTelemetryHash = hashTelemetry(telemetry);
        bool newTelemetry = newTelemetryHash != lastTelemetryHash;

        if (!connectionEstablished) {
            latestTelemetry = telemetry;
            lastTelemetryHash = newTelemetryHash;
            debugLogger.logf("Telemetry received: %.2fV, Speed: %d", telemetry.batteryVoltage, telemetry.speedRpm);
            connectionEstablished = true;
            connectionEstablishedAt = millis();
            screen.showConnectionEstablished();
        } else if (newTelemetry) {
            latestTelemetry = telemetry;
            lastTelemetryHash = newTelemetryHash;

            debugLogger.logf("Telemetry received: %.2fV, Speed: %d", telemetry.batteryVoltage, telemetry.speedRpm);

            if (debugScreen.isEnabled()) {
                debugScreen.showTelemetryReceived(telemetry.batteryVoltage, telemetry.speedRpm);
            } else if (dashboardShown) {
                showDashboard();
            }
        }
    }

    if (connectionEstablished && !dashboardShown && millis() - connectionEstablishedAt >= CONNECTION_MESSAGE_DURATION_MS) {
        dashboardShown = true;
        showDashboard();
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);  // Wait for serial monitor to connect

    screen.begin();
    initEspNow();
    addPeer(carMac);

#if CALIBRATION_MODE
    debugLogger.log("Calibration mode active");
    calibrationFlow.begin();
#else
    debugScreen.enable(DEBUG_SCREEN);
    debugLogger.log("RC Remote Starting...");
    screen.showStartup("Starting remote");
    printMacAddress();

    VehicleData probe = {JOYSTICK_CENTER_RAW, JOYSTICK_CENTER_RAW};
    debugLogger.log("Sending startup probe packet");
    sendData(probe, carMac);

    esp_now_register_recv_cb(onTelemetryReceive);
#endif
}

void loop() {
#if CALIBRATION_MODE
    {
        int rawX = readInput(JOY1_X_PIN);
        int rawY = readInput(JOY2_Y_PIN);
        VehicleData data = calibrationFlow.update(rawX, rawY);
        sendData(data, carMac);
        delay(20);
        return;
    }
#endif

    processTelemetryScreenUpdates();

    int joystickX = readInput(JOY1_X_PIN);
    int joystickY = readInput(JOY2_Y_PIN);

    static int prevJoyX = -1;
    static int prevJoyY = -1;
    static int lastSentX = -1;
    static int lastSentY = -1;
    static unsigned long lastSendTime = 0;
    static int consecX = 0;
    static int consecY = 0;
    const unsigned long SEND_REPEAT_MS = 100;

    if (prevJoyX < 0 || prevJoyY < 0) {
        prevJoyX = joystickX;
        prevJoyY = joystickY;
        lastSentX = joystickX;
        lastSentY = joystickY;
        lastSendTime = millis();
        consecX = 0;
        consecY = 0;
    }

    bool movedX = abs(joystickX - prevJoyX) > JOY_DEADZONE;
    bool movedY = abs(joystickY - prevJoyY) > JOY_DEADZONE;
    bool outsideNeutral = abs(joystickX - 2048) > JOY_DEADZONE || abs(joystickY - 2048) > JOY_DEADZONE;
    unsigned long now = millis();
    bool repeatSend = outsideNeutral && (now - lastSendTime >= SEND_REPEAT_MS);

    if (movedX) consecX++; else consecX = 0;
    if (movedY) consecY++; else consecY = 0;

    bool quickUpdate = consecX >= JOY_CONSECUTIVE_THRESHOLD || consecY >= JOY_CONSECUTIVE_THRESHOLD;
    bool changedSinceLastSend = abs(joystickX - lastSentX) > JOY_DEADZONE || abs(joystickY - lastSentY) > JOY_DEADZONE;
    bool directionChanged = (abs(lastSentY - 2048) > JOY_DEADZONE) && (abs(joystickY - 2048) > JOY_DEADZONE)
        && ((lastSentY < 2048) != (joystickY < 2048));

    if (quickUpdate || repeatSend || changedSinceLastSend || directionChanged) {
        debugLogger.logJoystick(joystickX, joystickY);
        debugScreen.showJoystickMoved(joystickX, joystickY);
        prevJoyX = joystickX;
        prevJoyY = joystickY;

        VehicleData data = {joystickX, joystickY};
        sendData(data, carMac);

        lastSentX = joystickX;
        lastSentY = joystickY;
        lastSendTime = now;
        consecX = 0;
        consecY = 0;
    }

    delay(20);
}

void onTelemetryReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len != sizeof(TelemetryData)) {
        return;
    }

    TelemetryData telemetry;
    memcpy(&telemetry, incomingData, sizeof(telemetry));

    portENTER_CRITICAL(&telemetryMux);
    pendingTelemetry = telemetry;
    pendingTelemetryAvailable = true;
    portEXIT_CRITICAL(&telemetryMux);
}
