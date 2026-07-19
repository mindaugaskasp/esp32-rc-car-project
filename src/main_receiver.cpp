#include <Arduino.h>
#include "config/DebugConfig.h"
#include "config/WifiConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/servo/ServoDriver.h"
#include "drivers/esc/EscDriver.h"
#include "drivers/hall/HallSensorDriver.h"
#include "drivers/battery/BatteryMonitorDriver.h"
#include "drivers/radio/EspNowDriver.h"
#include "comm/ChannelScanner.h"
#include "comm/ChannelAdvertiser.h"
#include "comm/VehicleCommandReceiver.h"

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500); // Wait for serial monitor to connect
    debugLogger.log("RC Car Starting...");
    printMacAddress();

    initServo();
    initEsc();
    initHallSensor();
    initBatteryMonitor();
    debugLogger.log("Servo, ESC, Hall sensor, and battery monitor initialized");

    initEspNow();

    // Camp on the rendezvous channel until the transmitter announces its channel.
    // Blocks here (vehicle stays neutral) rather than inventing a fallback channel.
    uint8_t channel = waitForChannelAdvertisement(TRANSMITTER_MAC);
    applyWifiChannel(channel);

    addPeer(TRANSMITTER_MAC);
    vehicleCommandReceiver.begin();
    // The "link established" servo twitch now fires on the first real command in
    // VehicleCommandReceiver, not here — channel sync alone isn't a confirmed link.
    debugLogger.log("Receiver ready and waiting for ESP-NOW packets");
}

void loop() {
    updateHallSensor();
    updateBatteryMonitor();
    vehicleCommandReceiver.update();

    // Bench aid, off by default; enable DEBUG_HALL_TO_SERIAL to verify the sensor
    // by spinning the wheel by hand. Self-gated and rate-limited inside the logger.
    debugLogger.logHallRpm(getMotorRpm());
}
