#pragma once
#include "comm/LinkModeLogic.h"

// Link Mode setting screen: shows the current radio PHY (Standard / Long Range)
// and whether a switch is in progress. Redrawn by LinkMode whenever the state
// changes, so it is passed the state rather than reading it itself.
class LinkModeScreen {
public:
    void show(LinkPhyMode applied, bool switching);
};

extern LinkModeScreen linkModeScreen;
