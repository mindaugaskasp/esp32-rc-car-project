#pragma once

#include <stdint.h>
#include "comm/DataTypes.h"

// Which axis a ResponseTuningScreen instance tunes. Throttle drives the ESC with
// the left (throttle) stick; Steering drives the servo with the right (steering)
// stick. In both, the *other* stick adjusts the selected parameter.
enum class TuningAxis : uint8_t { Throttle, Steering };

// Live "feel" tuner: the tuned actuator moves in real time while you shape its
// response. The drive stick commands the actuator through the in-progress deadzone
// + expo + rate; the free stick nudges the selected parameter; the steering-stick button short-taps to
// cycle Deadzone -> Expo -> Rate and holds to confirm. On confirm it prints the
// resulting #define lines to serial for copying into JoystickConfig.h.
class ResponseTuningScreen {
public:
    void begin(TuningAxis axis);
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    enum class Parameter : uint8_t { Deadzone, Expo, Rate };
    enum class State : uint8_t { Adjusting, Confirmed };

    TuningAxis _axis = TuningAxis::Throttle;
    State _state = State::Adjusting;
    Parameter _selected = Parameter::Deadzone;

    int _deadzone = 0;
    int _expo = 0;
    int _rate = 0;

    bool _steeringSwWas = false;
    bool _pressActive = false; // a rising steering-stick edge has started an in-progress press
    unsigned long _steeringSwDownAt = 0;
    unsigned long _lastAdjustMs = 0;

    static constexpr int DEADZONE_MIN = 0;
    static constexpr int DEADZONE_MAX = 400;
    static constexpr int DEADZONE_STEP = 2;
    static constexpr int CURVE_STEP = 10; // expo/rate step per adjust tick (per-mille)
    static constexpr int ADJUST_THRESHOLD_RAW = 800; // free-stick deflection to start adjusting
    static constexpr unsigned long ADJUST_INTERVAL_MS = 40; // repeat cadence while held
    static constexpr unsigned long CONFIRM_HOLD_MS = 1200;

    int driveAxisRaw(int rawX, int rawY) const;
    int adjustAxisRaw(int rawX, int rawY) const;
    int driveCenter() const;
    int adjustCenter() const;
    int conditionedCommand(int driveRaw) const;
    VehicleData buildCommand(int conditionedDrive, uint32_t now) const;

    void handleButton(bool steeringSw, unsigned long now);
    void cycleParameter();
    void applyAdjust(int adjustRaw, unsigned long now);
    void confirm();

    void showAdjusting(int conditionedDrive);
    void showConfirmed();
    void printResults() const;

    const char* axisTitle() const;
    const char* parameterName() const;
    int selectedValue() const;
    uint8_t parameterStep() const;
};
