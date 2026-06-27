#pragma once

#include <stdint.h>
#include "drivers/wifi/DataTypes.h"

enum class EscCalibStep : uint8_t {
    WaitForMax, HoldMax, WaitForMin, HoldMin, WaitForNeutral, Complete,
};

class EscRangeCalibScreen {
public:
    void begin();
    VehicleData update(int rawX, int rawY);
    bool isComplete() const;

private:
    EscCalibStep  _step = EscCalibStep::WaitForMax;
    unsigned long _stepEnteredAt = 0;
    int _detectedMax = 0;
    int _detectedMin = 0;

    void showStep(uint8_t stepNum, uint8_t totalSteps, const char* instruction, int rawY);
    void showComplete();
};
