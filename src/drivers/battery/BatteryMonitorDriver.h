#pragma once

void initBatteryMonitor();
void updateBatteryMonitor(); // call every loop(): non-blocking, samples on an interval
float getBatteryVoltage();   // smoothed pack voltage in volts; 0.0 until first sample
