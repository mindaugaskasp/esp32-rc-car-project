#include "ScreenDriverFactory.h"
#include "protocols/sh1106/Sh1106ScreenDriver.h"

std::unique_ptr<ScreenDriver> createScreenDriver() {
    return std::unique_ptr<ScreenDriver>(new Sh1106ScreenDriver());
}
