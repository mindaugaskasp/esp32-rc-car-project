#pragma once

// Vehicle drivetrain geometry — converts motor-shaft RPM (from the hall sensor)
// into ground speed for the dashboard. Kept as #define for native-test / Arduino
// parity, matching the other control-tuning configs (see CLAUDE.md).
//
// Consumed by SpeedLogic.h (motorRpmToKmh) via the ControlConfig.h umbrella. The
// km/h shown on the dashboard is only as accurate as these two numbers — measure
// them for your car. Changing either keeps the math correct; nothing else to edit.

// Drive-wheel outside diameter in millimetres (the tyre, not the rim).
#define WHEEL_DIAMETER_MM 65.0f

// Total reduction from motor shaft to wheel = motor revolutions per one wheel
// revolution (pinion→spur × any internal transmission/diff). motor_turns / wheel_turns.
#define GEAR_RATIO 8.0f
