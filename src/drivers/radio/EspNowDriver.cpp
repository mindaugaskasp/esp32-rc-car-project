#include "EspNowDriver.h"
#include <esp_now.h>
#include <WiFi.h>
#include "drivers/debug/DebugLogger.h"

static const char* espNowErrorToString(esp_err_t code) {
    switch (code) {
        case ESP_OK: return "ESP_OK";
        case ESP_ERR_ESPNOW_NOT_INIT: return "ESP_ERR_ESPNOW_NOT_INIT";
        case ESP_ERR_ESPNOW_ARG: return "ESP_ERR_ESPNOW_ARG";
        case ESP_ERR_ESPNOW_NO_MEM: return "ESP_ERR_ESPNOW_NO_MEM";
        case ESP_ERR_ESPNOW_FULL: return "ESP_ERR_ESPNOW_FULL";
        case ESP_ERR_ESPNOW_NOT_FOUND: return "ESP_ERR_ESPNOW_NOT_FOUND";
        case ESP_ERR_ESPNOW_INTERNAL: return "ESP_ERR_ESPNOW_INTERNAL";
        case ESP_ERR_ESPNOW_EXIST: return "ESP_ERR_ESPNOW_EXIST";
        case ESP_ERR_ESPNOW_IF: return "ESP_ERR_ESPNOW_IF";
        default: return "ESP_ERR_UNKNOWN";
    }
}

void initEspNow() {
    WiFi.mode(WIFI_STA);
    esp_err_t initResult = esp_now_init();
    if (initResult != ESP_OK) {
        debugLogger.logf("Failed to initialize ESP-NOW: %d", initResult);
    } else {
        debugLogger.log("ESP-NOW initialized");
    }
}


void addPeer(const uint8_t* macAddress) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;
    
    esp_err_t peerResult = esp_now_add_peer(&peerInfo);
    if (peerResult != ESP_OK) {
        char peerMac[18];
        snprintf(peerMac, sizeof(peerMac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
        debugLogger.logf("Failed to add peer %s: %s (%d)", peerMac, espNowErrorToString(peerResult), peerResult);
    } else {
        char peerMac[18];
        snprintf(peerMac, sizeof(peerMac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
        debugLogger.logf("Peer added: %s", peerMac);
    }
}

void printMacAddress() {
#ifdef IS_TRANSMITTER
    debugLogger.logf("Transmitter MAC: %s", WiFi.macAddress().c_str());
#elif defined(IS_RECEIVER)
    debugLogger.logf("Receiver MAC: %s", WiFi.macAddress().c_str());
#else
    debugLogger.logf("Device MAC: %s", WiFi.macAddress().c_str());
#endif
}

#ifdef IS_TRANSMITTER
void sendData(VehicleData data, const uint8_t* mac) {
    esp_err_t result = esp_now_send(mac, reinterpret_cast<const uint8_t*>(&data), sizeof(data));
    if (result != ESP_OK) {
        debugLogger.logf("ESP-NOW send failed: %d", result);
    }
}
#endif
