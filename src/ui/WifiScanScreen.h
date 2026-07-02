#pragma once
#include <comm/ChannelScanner.h>
#include <stdint.h>

class WifiScanScreen {
public:
    void showScanning();
    void showResult(const ChannelScanResult& result, int countdownSecs, bool broadcasting = false);
private:
    uint8_t _spinnerFrame = 0;
};

extern WifiScanScreen wifiScanScreen;
