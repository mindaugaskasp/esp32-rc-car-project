#include "ScreenDriverFactory.h"
#include "protocols/sh1106/Sh1106ScreenDriver.h"

ScreenDriver* createScreenDriver() {
    static Sh1106ScreenDriver instance;
    return &instance;
}
