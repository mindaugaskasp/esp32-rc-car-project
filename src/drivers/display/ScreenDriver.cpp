#include "ScreenDriver.h"
#include "ScreenDriverFactory.h"

static ScreenDriver* screenDriver = nullptr;
static bool screenDriverResolved = false;

ScreenDriver* getScreenDriver() {
    // Resolve once: createScreenDriver() may legitimately return nullptr (no panel
    // on the bus → headless). Guard with a flag so a null result doesn't re-probe
    // I2C on every draw call.
    if (!screenDriverResolved) {
        screenDriver = createScreenDriver();
        screenDriverResolved = true;
    }
    return screenDriver;
}
