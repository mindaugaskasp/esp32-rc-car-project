#pragma once

// Serial logging gates — set to false to silence subsystem logs over USB.
#define DEBUG_JOYSTICK_TO_SERIAL         true
#define CALIBRATION_DEBUG_SCREEN_ENABLED true

// Minimum time between repeated [JOY] / Telemetry log lines, so the serial
// monitor stays readable instead of scrolling by every ~20-100ms. Raise this
// for an even slower feed while eyeballing wiring/calibration values.
#define DEBUG_LOG_MIN_INTERVAL_MS 500

// When true, the Dashboard's small link-quality indicator rotates between
// latency, packet loss %, and jitter every DASHBOARD_STAT_DWELL_MS instead of
// only ever showing latency.
#define DEBUG_DASHBOARD_LINK_STATS true
#define DASHBOARD_STAT_DWELL_MS    2500
