#include "ScreenDriver.h"
#include "ScreenDriverFactory.h"

static ScreenDriver* screenDriver = nullptr;

ScreenDriver* getScreenDriver() {
    if (!screenDriver) {
        screenDriver = createScreenDriver();
    }
    return screenDriver;
}
