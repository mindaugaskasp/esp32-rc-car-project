#include "ChannelAdvertiser.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include <drivers/debug/DebugLogger.h>

static const uint8_t ADVERT_MAGIC        = 0xCA;
static const uint8_t BROADCAST_MAC[6]    = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct ChannelAdvertPacket {
    uint8_t magic;
    uint8_t channel;
};
static_assert(sizeof(ChannelAdvertPacket) == 2, "ChannelAdvertPacket wire size changed");

// ── Transmitter side ──────────────────────────────────────────────────────────

void initChannelBroadcast() {
    esp_wifi_set_channel(ADVERT_CHANNEL, WIFI_SECOND_CHAN_NONE);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST_MAC, 6);
    peer.channel = ADVERT_CHANNEL;
    peer.ifidx   = WIFI_IF_STA;
    esp_now_add_peer(&peer);

    debugLogger.logf("[ADVERT] Broadcasting on ch %d", ADVERT_CHANNEL);
}

void sendChannelAdvert(uint8_t channel) {
    ChannelAdvertPacket pkt = {ADVERT_MAGIC, channel};
    esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
}

void stopChannelBroadcast() {
    esp_now_del_peer(BROADCAST_MAC);
    debugLogger.log("[ADVERT] Broadcast stopped");
}

// ── Receiver side ─────────────────────────────────────────────────────────────

static portMUX_TYPE         advertMux       = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t     receivedChannel = 0;
static volatile bool        advertReceived  = false;
static const uint8_t*       expectedAdvertMac = nullptr;

static void onAdvertPacket(const uint8_t* mac, const uint8_t* data, int len) {
    if ((size_t)len < sizeof(ChannelAdvertPacket)) return;
    if (expectedAdvertMac && memcmp(mac, expectedAdvertMac, 6) != 0) return;
    const ChannelAdvertPacket* pkt = reinterpret_cast<const ChannelAdvertPacket*>(data);
    if (pkt->magic != ADVERT_MAGIC) return;
    portENTER_CRITICAL(&advertMux);
    receivedChannel = pkt->channel;
    advertReceived  = true;
    portEXIT_CRITICAL(&advertMux);
}

uint8_t receiveChannelAdvert(const uint8_t* expectedTransmitterMac, uint32_t timeoutMs, uint8_t fallbackChannel) {
    esp_wifi_set_channel(ADVERT_CHANNEL, WIFI_SECOND_CHAN_NONE);

    portENTER_CRITICAL(&advertMux);
    receivedChannel   = 0;
    advertReceived    = false;
    expectedAdvertMac = expectedTransmitterMac;
    portEXIT_CRITICAL(&advertMux);

    esp_now_register_recv_cb(onAdvertPacket);

    debugLogger.logf("[ADVERT] Listening on ch %d (timeout %lums)", ADVERT_CHANNEL, (unsigned long)timeoutMs);

    unsigned long startTime = millis();
    bool received = false;
    uint8_t channel = 0;
    while (millis() - startTime < timeoutMs) {
        portENTER_CRITICAL(&advertMux);
        received = advertReceived;
        channel  = receivedChannel;
        portEXIT_CRITICAL(&advertMux);
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
