#pragma once
#include "sdm630.h"

struct DeviceStats {
  uint32_t uptime = 0;
  uint32_t freeHeap = 0;
  int32_t wifiRSSI = 0;
  uint32_t influxErrors = 0;
  uint32_t influxWrites = 0;
};

bool influxBegin();
bool influxSend(const SDM630Data &d, const DeviceStats &sys, const ModbusStats &mb);
