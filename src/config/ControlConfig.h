#pragma once

// Aggregate control-tuning header. The actual constants now live in per-subsystem
// files so each is easy to find and edit in isolation; this umbrella pulls them all
// in so existing `#include "config/ControlConfig.h"` sites keep working unchanged
// (and can serve as the single bundle a future reference class wraps).
//
// Edit values in the focused file for the subsystem you are tuning:
//   JoystickConfig.h — stick centers, deadzones, inversion, travel endpoints (X & Y)
//   ServoConfig.h    — steering-servo PWM range, smoothing, jitter deadband
//   EscConfig.h      — ESC output deadband, throttle-invert
//   HallConfig.h     — hall speed-sensor pulses/rev, RPM window, debounce
//   VehicleConfig.h  — drivetrain geometry (wheel diameter, gear ratio) for km/h
//   BatteryConfig.h  — battery-sense divider values, sampling, smoothing
//
// All remain #define (not constexpr) for native-test / Arduino parity — see CLAUDE.md.

#include "config/JoystickConfig.h"
#include "config/ServoConfig.h"
#include "config/EscConfig.h"
#include "config/HallConfig.h"
#include "config/VehicleConfig.h"
#include "config/BatteryConfig.h"
