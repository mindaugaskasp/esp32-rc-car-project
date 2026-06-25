#include <Arduino.h>
#include "drivers/servo/ServoDriver.h"
#include "drivers/esc/EscDriver.h"
#include "drivers/wifi/EspNowDriver.h"
#include <esp_now.h>

void onDataReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len != sizeof(VehicleData)) {
        return; 
    }

    VehicleData data;
    memcpy(&data, incomingData, sizeof(data));
    
    updateServo(data.servoPos);
    updateEscSpeed(data.escSpeed);
    
    TelemetryData telemetry;
    telemetry.batteryVoltage = 7.4; // Replace with actual voltage function
    telemetry.speedRpm = 50;        // Replace with actual speed function
    
    //Send back to the sender's MAC address
    esp_err_t result = esp_now_send(mac, (uint8_t *)&telemetry, sizeof(telemetry));
    
    if (result != ESP_OK) {
        // Optional: Log error if transmission fails
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    
    initEspNow();
    initEspNowReceiver();
}

void loop() {
    // Keep empty: event-driven architecture handles everything via callbacks
}