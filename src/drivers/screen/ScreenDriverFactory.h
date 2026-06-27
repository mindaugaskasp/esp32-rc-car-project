#pragma once

#include "ScreenDriver.h"
#include <memory>

std::unique_ptr<ScreenDriver> createScreenDriver();
