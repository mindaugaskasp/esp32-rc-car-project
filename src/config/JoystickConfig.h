#pragma once

// Raw ADC characteristics of both sticks (rest centers, deadzones, inversion,
// travel endpoints). Consumed by the servo/ESC maps, the send-decision logic,
// and the on-screen calibration. Run the on-device Joystick Calibration to get
// tuned values — it prints suggested #defines under the [JOY CAL] serial tag.
// Kept as #define (not constexpr) for native-test/Arduino parity — see CLAUDE.md.

// Steering (X) stick rest point, in ADC units. The physical stick does NOT rest
// at the nominal ADC_MIDPOINT_RAW; set this from calibration. Also the servo-center
// reference.
#define STEERING_CENTER_RAW 1852

// Per-axis band around the rest point where movement is ignored. Keep just above
// the stick's resting jitter: too low and a released stick drifts past it (servo
// twitch / motor creep); too high and small inputs feel dead. Y needs margin because
// throttle creep drives the motor, whereas X drift only jitters the servo.
#define JOY_DEADZONE_X 70
#define JOY_DEADZONE_Y 45

// Consecutive samples that must exceed the deadzone before sending.
#define JOY_CONSECUTIVE_THRESHOLD 3

// Full-scale 12-bit ADC reading; pivot for axis inversion.
#define ADC_MAX_RAW 4095

// Nominal 12-bit ADC midpoint — a display/gesture reference only, NOT the stick
// rest point (sticks rest at STEERING_CENTER_RAW / THROTTLE_CENTER_RAW).
#define ADC_MIDPOINT_RAW 2048

// Y-stick gesture thresholds (raw ADC): above UP or below DOWN registers a menu tap.
#define JOY_GESTURE_UP_RAW 3500
#define JOY_GESTURE_DOWN_RAW 500

// Per-axis inversion, applied to the raw reading before deadzone/mapping so control
// and menu navigation share one orientation. This is the axis-level flip; THROTTLE_INVERT
// (EscConfig.h) mirrors only the ESC output — enabling both on throttle cancels out.
#define JOY_INVERT_X false
#define JOY_INVERT_Y true

// Steering (X) full-deflection endpoints, in ADC units. Defaults are the full range;
// tighten to the logged travel[min..max] for fuller servo throw before the stops.
#define STEERING_JOY_MIN 0
#define STEERING_JOY_MAX 4095

// Throttle (Y) rest point and full-deflection endpoints, in ADC units. The ESC neutral
// band is centered on THROTTLE_CENTER_RAW, so an accurate value is what keeps the motor
// stopped at rest — set it from calibration, not the nominal ADC_MIDPOINT_RAW.
#define THROTTLE_CENTER_RAW 2252
#define THROTTLE_JOY_MIN 0
#define THROTTLE_JOY_MAX 4095

// Per-axis response curve applied on the transmitter before send (see
// InputConditioningLogic.h). Per-mille, 0..1000:
//   *_EXPO — 0 = linear; higher softens response near center, endpoint unchanged.
//   *_RATE — 1000 = full travel; lower caps the top end.
// Defaults reproduce the original linear, full-range behavior.
#define STEERING_EXPO 0
#define STEERING_RATE 1000
#define THROTTLE_EXPO 0
#define THROTTLE_RATE 1000
