#pragma once

#include <Arduino.h>
#include "modbus_sdm630.h"

struct SDM630Data {
  bool online = false;
  bool valid = false;
  uint32_t qualityFlags = 0;

  float voltageA=0, voltageB=0, voltageC=0;
  float lineVoltageAB=0, lineVoltageBC=0, lineVoltageCA=0, lineVoltageAvg=0;
  float currentA=0, currentB=0, currentC=0, currentNeutral=0, currentAvg=0, currentSum=0;
  float powerA=0, powerB=0, powerC=0, powerTotal=0;
  float reactiveA=0, reactiveB=0, reactiveC=0, reactiveTotal=0;
  float apparentA=0, apparentB=0, apparentC=0, apparentTotal=0;
  float pfA=0, pfB=0, pfC=0, pfTotal=0;
  float angleA=0, angleB=0, angleC=0, angleTotal=0;
  float frequencyHz=0;

  float importKWh=0, exportKWh=0, importKVARh=0, exportKVARh=0, totalKVAh=0, ampHours=0;
  float powerDemandW=0, maxPowerDemandW=0;
  float vaDemand=0, maxVaDemand=0, neutralDemandA=0, maxNeutralDemandA=0;
  float reactiveDemandVar=0, maxReactiveDemandVar=0;

  float thdVoltageA=0, thdVoltageB=0, thdVoltageC=0;
  float thdCurrentA=0, thdCurrentB=0, thdCurrentC=0;
  float thdVoltageAvg=0, thdCurrentAvg=0, thdVoltageAB=0, thdVoltageBC=0, thdVoltageCA=0, thdVoltageLineAvg=0;

  float currentDemandA=0, currentDemandB=0, currentDemandC=0;
  float maxCurrentDemandA=0, maxCurrentDemandB=0, maxCurrentDemandC=0;

  float totalKWh=0, totalKVARh=0;
  float importKWhA=0, importKWhB=0, importKWhC=0;
  float exportKWhA=0, exportKWhB=0, exportKWhC=0;
  float totalKWhA=0, totalKWhB=0, totalKWhC=0;
  float importKVARhA=0, importKVARhB=0, importKVARhC=0;
  float exportKVARhA=0, exportKVARhB=0, exportKVARhC=0;
  float totalKVARhA=0, totalKVARhB=0, totalKVARhC=0;

  float resetTotalKWh=0, resetTotalKVARh=0, resetImportKWh=0, resetExportKWh=0;
  float resetImportKVARh=0, resetExportKVARh=0;
};

class SDM630 {
public:
  SDM630(SDM630Modbus &bus) : _bus(bus) {}
  bool read(SDM630Data &d);

private:
  SDM630Modbus &_bus;
  uint16_t r[80];
  float f(const uint16_t *buf, uint16_t index) const;
  bool block(uint16_t start, uint16_t count, uint16_t *dst);
  void map(SDM630Data &d);
  bool validate(SDM630Data &d);
};
