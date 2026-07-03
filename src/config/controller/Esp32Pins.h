#pragma once

#if defined(IS_TRANSMITTER)
#include "Esp32PinsTransmitter.h"
#elif defined(IS_RECEIVER)
#include "Esp32PinsReceiver.h"
#else
#error "No target defined. Set IS_TRANSMITTER or IS_RECEIVER in build flags."
#endif
