#include "ChannelAdvertiser.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include <drivers/debug/DebugLogger.h>

static const uint8_t ADVERTISEMENT_MAGIC = 0xCA;
static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct ChannelAdvertisementPacket {
    uint8_t magic;
    uint8_t channel;
};
static_assert(sizeof(ChannelAdvertisementPacket) == 2, "ChannelAdvertisementPacket wire size changed");

// ── Transmitter side ──────────────────────────────────────────────────────────

void initChannelBroadcast() {
    esp_wifi_set_channel(ADVERTISEMENT_CHANNEL, WIFI_SECOND_CHAN_NONE);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST_MAC, 6);
    peer.channel = ADVERTISEMENT_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    esp_now_add_peer(&peer);

    debugLogger.logf("[ADVERT] Broadcasting on ch %d", ADVERTISEMENT_CHANNEL);
}

void sendChannelAdvertisement(uint8_t channel) {
    ChannelAdvertisementPacket pkt = {ADVERTISEMENT_MAGIC, channel};
    esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
}

void stopChannelBroadcast() {
    esp_now_del_peer(BROADCAST_MAC);
    debugLogger.log("[ADVERT] Broadcast stopped");
}

// ── Receiver side ─────────────────────────────────────────────────────────────

static portMUX_TYPE advertisementMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t receivedChannel = 0;
static volatile bool advertisementReceived = false;
static const uint8_t* expectedAdvertisementMac = nullptr;

static void onAdvertisementPacket(const uint8_t* mac, const uint8_t* data, int len) {
    if ((size_t)len < sizeof(ChannelAdvertisementPacket)) return;
    if (expectedAdvertisementMac && memcmp(mac, expectedAdvertisementMac, 6) != 0) return;
    const ChannelAdvertisementPacket* pkt = reinterpret_cast<const ChannelAdvertisementPacket*>(data);
    if (pkt->magic != ADVERTISEMENT_MAGIC) return;
    portENTER_CRITICAL(&advertisementMux);
    receivedChannel = pkt->channel;
    advertisementReceived = true;
    portEXIT_CRITICAL(&advertisementMux);
}

uint8_t receiveChannelAdvertisement(const uint8_t* expectedTransmitterMac, uint32_t timeoutMs, uint8_t fallbackChannel) {
    esp_wifi_set_channel(ADVERTISEMENT_CHANNEL, WIFI_SECOND_CHAN_NONE);

    portENTER_CRITICAL(&advertisementMux);
    receivedChannel = 0;
    advertisementReceived = false;
    expectedAdvertisementMac = expectedTransmitterMac;
    portEXIT_CRITICAL(&advertisementMux);

    esp_now_register_recv_cb(onAdvertisementPacket);

    debugLogger.logf("[ADVERT] Listening on ch %d (timeout %lums)", ADVERTISEMENT_CHANNEL, (unsigned long)timeoutMs);

    unsigned long startTime = millis();
    bool received = false;
    uint8_t channel = 0;
    while (millis() - startTime < timeoutMs) {
        portENTER_CRITICAL(&advertisementMux);
        received = advertisementReceived;
        channel = receivedChannel;
        portEXIT_CRITICAL(&advertisementMux);
        if (received) break;
        delay(50);
    }

    esp_now_unregister_recv_cb();

    if (received) {
        debugLogger.logf("[ADVERT] Got channel: %d", channel);
        return channel;
    }

    debugLogger.logf("[ADVERT] Timeout — falling back to ch %d", fallbackChannel);
    return fallbackChannel;
}
