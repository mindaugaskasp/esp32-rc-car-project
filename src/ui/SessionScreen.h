#pragma once
#include "app/SessionStatsLogic.h"

// Renders the session Data mode: a numeric stats page (distance, time, average
// and max speed) and a speed-over-time graph page. Draws via the display driver;
// owns no state.
class SessionScreen {
public:
    void showStats(const SessionStats& stats);
    void showGraph(const SpeedHistory& history, float maxSpeedKmh);
};

extern SessionScreen sessionScreen;
