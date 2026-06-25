#include "EspNowDriver.h"
#include <esp_now.h>
#include <WiFi.h>

void initEspNow() {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Failed to initialize ESP-NOW");
    }
}


void addPeer(const uint8_t* macAddr) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
    }
}

void printMacAddress() {
    Serial.print("Device MAC: ");
    Serial.println(WiFi.macAddress());
}

#ifdef IS_TRANSMITTER
void sendData(VehicleData data, const uint8_t* mac) {
    esp_now_send(mac, (uint8_t *) &data, sizeof(data));
}
#endif

#ifdef IS_RECEIVER
// Čia nurodome, kad funkcija bus apibrėžta main_receiver.cpp
extern void onDataReceive(const uint8_t * mac, const uint8_t *incomingData, int len);

void initEspNowReceiver() {
    esp_now_register_recv_cb(onDataReceive);
}
#endif