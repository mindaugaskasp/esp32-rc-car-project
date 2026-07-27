#pragma once

// Serial logging gates — set to false to silence subsystem logs over USB. These
// stay compile-time flags, independent of the transmitter's runtime DEBUG switch:
// DEBUG_JOYSTICK_TO_SERIAL logs raw stick reads on the transmitter, and
// DEBUG_ESC_TO_SERIAL / DEBUG_HALL_TO_SERIAL log ESC output micros and hall RPM on
// the headless receiver — which has neither a screen nor a physical switch, so a
// compile-time gate is all it needs. DEBUG_HALL_TO_SERIAL is a bench aid for
// verifying the hall sensor by spinning the wheel by hand (rate-limited well below
// the packet trace, so it never stalls the loop).
#define DEBUG_JOYSTICK_TO_SERIAL true
#define DEBUG_ESC_TO_SERIAL true
#define DEBUG_HALL_TO_SERIAL false

// Per-packet trace on the ESP-NOW hot path (receiver command receipt + telemetry
// echo, transmitter telemetry receipt). MUST stay false during normal operation:
// these run on every packet, and at the full command rate the 115200 UART can't
// keep up, so Serial.println() blocks and stalls the loop — adding hundreds of ms
// of control latency to BOTH the servo and ESC. On the transmitter this is the
// compile-time default for the runtime trace toggle (flip it in the Debug menu
// for a short bench window); on the headless receiver it is the compile-time gate.
#define DEBUG_PACKET_TRACE false

// Minimum time between repeated [JOY] / Telemetry log lines, so the serial
// monitor stays readable instead of scrolling by every ~20-100ms. Raise this
// for an even slower feed while eyeballing wiring/calibration values.
#define DEBUG_LOG_MIN_INTERVAL_MS 0

// Dwell time per rotating dashboard link-stat (latency / loss / jitter) shown in
// the battery row while the transmitter's physical DEBUG switch is ON (see
// DEBUG_MODE_SWITCH_PIN in config/controller/Esp32PinsTransmitter.h).
#define DASHBOARD_STAT_DWELL_MS 2500
