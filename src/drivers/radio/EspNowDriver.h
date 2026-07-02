#pragma once
#include <Arduino.h>
#include "comm/DataTypes.h"

void sendData(VehicleData data, const uint8_t* mac);
void initEspNow();
void addPeer(const uint8_t* macAddr);
void printMacAddress();