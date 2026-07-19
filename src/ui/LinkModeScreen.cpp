#include "LinkModeScreen.h"
#include "drivers/display/ScreenDriver.h"

LinkModeScreen linkModeScreen;

void LinkModeScreen::show(LinkPhyMode applied, bool switching) {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    driver->clear();
    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "LINK MODE");
    driver->hline(0, 13, ScreenDriver::W);

    driver->font(ScreenFont::Large);
    driver->text(4, 37, applied == LinkPhyMode::LongRange ? "LONG RANGE" : "STANDARD");

    driver->font(ScreenFont::Small);
    if (switching) {
        driver->text(0, 52, "Switching...");
    } else {
        driver->text(0, 52, applied == LinkPhyMode::LongRange ? "Extended range" : "Normal range");
    }

    driver->hline(0, 55, ScreenDriver::W);
    driver->font(ScreenFont::Tiny);
    driver->text(0, 63, "STEER:toggle  THR:exit");

    driver->flush();
}
