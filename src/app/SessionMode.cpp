#include "SessionMode.h"
#include "app/SessionTracker.h"
#include "ui/SessionScreen.h"
#include "config/controller/Esp32Pins.h"
#include "drivers/controls/Controls.h"
#include <Arduino.h>

SessionMode sessionMode;

void SessionMode::begin() {
    _view = View::Stats;
    _needsRedraw = true;
    _wantsExit = false;
    _throttleSwPressActive = false;
    _throttleSwDownAt = 0;
    // Require SW release before back/exit registers — the steering press that selected
    // this mode from the menu is likely still held on the first frame.
    _throttleSwWas = readButton(THROTTLE_SW_PIN);
    _steeringSwWas = readButton(STEERING_SW_PIN);
}

void SessionMode::draw() {
    if (_view == View::Stats) {
        sessionScreen.showStats(sessionTracker.stats());
    } else {
        sessionScreen.showGraph(sessionTracker.history(), sessionTracker.stats().maxSpeedKmh);
    }
}

void SessionMode::update() {
    if (_needsRedraw) {
        draw();
        _needsRedraw = false;
    }

    bool throttleSw = readButton(THROTTLE_SW_PIN);
    bool steeringSw = readButton(STEERING_SW_PIN);

    if (_view == View::Stats) {
        handleStatsButtons(throttleSw, steeringSw, millis());
    } else if (throttleSw && !_throttleSwWas) {
        _view = View::Stats;
        _needsRedraw = true;
    }

    _throttleSwWas = throttleSw;
    _steeringSwWas = steeringSw;
}

// Stats page: steering tapped opens the graph; throttle held past RESET_HOLD_MS resets the
// session (firing on the threshold while still held), a shorter throttle tap exits.
void SessionMode::handleStatsButtons(bool throttleSw, bool steeringSw, unsigned long now) {
    if (steeringSw && !_steeringSwWas) {
        _view = View::Graph;
        _needsRedraw = true;
        return;
    }

    if (throttleSw && !_throttleSwWas) {
        _throttleSwDownAt = now;
        _throttleSwPressActive = true;
    }
    if (_throttleSwPressActive && throttleSw && (now - _throttleSwDownAt >= RESET_HOLD_MS)) {
        _throttleSwPressActive = false;
        sessionTracker.begin();
        _needsRedraw = true;
    } else if (_throttleSwPressActive && !throttleSw) {
        if (now - _throttleSwDownAt < RESET_HOLD_MS) _wantsExit = true;
        _throttleSwPressActive = false;
    }
}

bool SessionMode::wantsExit() {
    bool result = _wantsExit;
    _wantsExit = false;
    return result;
}
