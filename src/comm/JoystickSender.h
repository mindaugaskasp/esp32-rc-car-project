#pragma once
#include <stdint.h>
#include "comm/DataTypes.h"

// Decides when to actually transmit joystick data: repeat-suppression while
// idle, forced periodic resend while off-center, and quick-update bursts
// during fast stick movement. See send() for the exact rules.
class JoystickSender {
public:
    // Sends (or suppresses) one joystick sample. centerRaw is the assumed ADC
    // rest point for both axes, used by the outside-neutral/direction-change checks.
    void send(int joystickX, int joystickY, int centerRaw, const uint8_t* mac);

    uint32_t getSentCount() const { return _sentCount; }

private:
    static const unsigned long SEND_REPEAT_MS = 100;

    int _prevJoyX = -1, _prevJoyY = -1;
    int _lastSentX = -1, _lastSentY = -1;
    unsigned long _lastSendTime = 0;
    int _consecX = 0, _consecY = 0;
    uint32_t _sentCount = 0;
};

extern JoystickSender joystickSender;
