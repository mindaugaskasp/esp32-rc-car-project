#include "ScreenDriver.h"
#include "ScreenDriverFactory.h"
#include <memory>

static std::unique_ptr<ScreenDriver> screenDriver;

ScreenDriver* getScreenDriver() {
    if (!screenDriver) {
        screenDriver = createScreenDriver();
    }
    return screenDriver.get();
}
