#pragma once

// Hall effect sensor driver for motor RPM measurement.
//
// Tested with KY-003 / 3144 module (active-low digital output).
// The module has a built-in pull-up resistor and indicator LED.
//
// Wiring:
//   VCC  → 3.3 V  (module regulates internally; do NOT use 5 V on ESP32)
//   GND  → GND
//   S    → HALL_SENSOR_PIN (GPIO 25)
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
int  getMotorRpm();
