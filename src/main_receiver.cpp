#include <Arduino.h>
#include "config/DebugConfig.h"
#include "config/WifiConfig.h"
#include "drivers/debug/DebugLogger.h"
#include "drivers/debug/StatusLedDriver.h"
#include "drivers/servo/ServoDriver.h"
#include "drivers/esc/EscDriver.h"
#include "drivers/hall/HallSensorDriver.h"
#include "drivers/battery/BatteryMonitorDriver.h"
#include "drivers/radio/EspNowDriver.h"
#include "comm/ChannelScanner.h"
#include "comm/ChannelAdvertiser.h"
#include "comm/VehicleCommandReceiver.h"

// Status LED: white while booting, green blink once running but with no transmitter
// linked, blue blink once commands are flowing. See docs/status-led.md.
static constexpr uint16_t STATUS_BLINK_PERIOD_MS = 250;

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500); // Wait for serial monitor to connect
    initStatusLed();
    setStatusLed(StatusColor::White);
    debugLogger.log("RC Car Starting...");
    printMacAddress();

    initServo();
    initEsc();
    initHallSensor();
    initBatteryMonitor();
    debugLogger.log("Servo, ESC, Hall sensor, and battery monitor initialized");

    initEspNow();

    blinkStatusLed(StatusColor::Green, STATUS_BLINK_PERIOD_MS); // running; no transmitter link yet

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

    // Re-arm the blink only when the link state flips, so the phase isn't reset
    // every tick (which would hold the LED on a single colour).
    static bool lastLinkAlive = false;
    const bool linkAlive = vehicleCommandReceiver.isLinkAlive();
    if (linkAlive != lastLinkAlive) {
        lastLinkAlive = linkAlive;
        blinkStatusLed(linkAlive ? StatusColor::Blue : StatusColor::Green, STATUS_BLINK_PERIOD_MS);
    }
    updateStatusLed();

    // Bench aid, off by default; enable DEBUG_HALL_TO_SERIAL to verify the sensor
    // by spinning the wheel by hand. Self-gated and rate-limited inside the logger.
    debugLogger.logHallRpm(getMotorRpm());
}
