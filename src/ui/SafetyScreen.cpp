#include "SafetyScreen.h"
#include "drivers/display/ScreenDriver.h"

SafetyScreen safetyScreen;

void SafetyScreen::show() {
    ScreenDriver* driver = getScreenDriver();
    if (!driver) return;

    driver->clear();
    driver->font(ScreenFont::Medium);
    driver->text(0, 10, "SAFETY STOP");
    driver->hline(0, 13, ScreenDriver::W);

    driver->font(ScreenFont::Large);
    driver->text(12, 37, "MOTOR STOPPED");

    driver->font(ScreenFont::Small);
    driver->text(0, 52, "Forcing ESC neutral");

    driver->hline(0, 55, ScreenDriver::W);
    driver->font(ScreenFont::Tiny);
    driver->text(0, 63, "THR:exit  stick ignored");

    driver->flush();
}
