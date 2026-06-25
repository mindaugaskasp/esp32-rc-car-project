#include <Arduino.h>
#include "config/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include "drivers/wifi/EspNowDriver.h"
#include <esp_now.h>

void onTelemetryReceive(const uint8_t * mac, const uint8_t *incomingData, int len);

uint8_t carMac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

void setup() {
    Serial.begin(BAUD_RATE);
    initEspNow();
    addPeer(carMac);
    
    // Dabar kompiliatorius jau žino, kas yra onTelemetryReceive
    esp_now_register_recv_cb(onTelemetryReceive);
}

void loop() {
    VehicleData data = {readInput(JOY1_X_PIN), readInput(JOY2_Y_PIN)};
    sendData(data, carMac);
    delay(20);
}

void onTelemetryReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
    TelemetryData t;
    memcpy(&t, incomingData, sizeof(t));
    
    Serial.printf("Telemetry received: %.2fV, Speed: %d\n", t.batteryVoltage, t.speedRpm);
}