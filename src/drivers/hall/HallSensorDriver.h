#pragma once

// Hall effect sensor driver for motor RPM measurement.
//
// Tested with KY-003 / 3144 module (active-low digital output).
// The module has a built-in pull-up resistor and indicator LED.
//
// Wiring:
//   VCC  → 5 V  (sensor rated 4.5–24 V; 3.3 V is below spec → false readings)
//   GND  → GND
//   S    → 10 kΩ → GPIO 25, and 20 kΩ from GPIO 25 to GND
//          (voltage divider: 5 V × 20/30 = 3.33 V, safe for ESP32 3.3 V GPIO)
//
// The 3144 is a UNIPOLAR sensor — it only responds to the SOUTH pole of a
// magnet. Hold the correct face of your magnet toward the sensor dot.
//
// PULSES_PER_REV guidance:
//   - Single magnet on shaft / gear tooth    → 1
//   - 4-pole brushless motor (2 pole pairs)  → 2  (rotor south poles pass sensor)
//   - Adjust in ControlConfig.h (HALL_PULSES_PER_REV)
//
// Each FALLING edge (magnet detected) is counted as one pulse. RPM is
// recalculated every HALL_RPM_INTERVAL_MS from the accumulated pulse count.
//
// Call initHallSensor() once in setup() and updateHallSensor() every loop().

void initHallSensor();
void updateHallSensor();
int getMotorRpm();
