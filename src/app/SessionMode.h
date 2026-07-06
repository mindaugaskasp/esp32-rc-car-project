#pragma once
#include <stdint.h>

// Session Data mode: shows the accumulated session stats (distance, time,
// average/max speed) and, on steering, a speed-over-time graph page. On the stats
// page throttle tapped exits to the mode menu, throttle held resets the session; on the
// graph page throttle steps back to the stats page. Reads its data from
// sessionTracker; drives nothing. Mirrors WifiPingMode's begin()/update()/
// wantsExit() pattern.
class SessionMode {
public:
    void begin();
    void update();
    // Returns true (once) when the user exits with a throttle tap from the stats page.
    bool wantsExit();

private:
    enum class View : uint8_t { Stats, Graph };

    // Hold throttle at least this long on the stats page to reset the session; a
    // shorter tap exits instead (same tap-vs-hold idiom as ResponseTuningScreen).
    static const unsigned long RESET_HOLD_MS = 800;

    void draw();
    void handleStatsButtons(bool throttleSw, bool steeringSw, unsigned long now);

    View _view = View::Stats;
    bool _needsRedraw = true;
    bool _throttleSwWas = false;
    bool _steeringSwWas = false;
    bool _throttleSwPressActive = false;
    unsigned long _throttleSwDownAt = 0;
    bool _wantsExit = false;
};

extern SessionMode sessionMode;
