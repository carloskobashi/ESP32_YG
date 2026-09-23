#include "influx_manager.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>

static InfluxDBClient influx(
  INFLUXDB_URL,
  INFLUXDB_ORG,
  INFLUXDB_BUCKET,
  INFLUXDB_TOKEN);

bool influxBegin() {
  if (WiFi.status() != WL_CONNECTED) return false;
  return influx.validateConnection();
}

static void addAllFields(Point &p, const SDM630Data &d, const ModbusStats &mb) {
#define F(name) p.addField(#name, d.name)
  F(voltageA); F(voltageB); F(voltageC);
  F(lineVoltageAB); F(lineVoltageBC); F(lineVoltageCA); F(lineVoltageAvg);
  F(currentA); F(currentB); F(currentC); F(currentNeutral); F(currentAvg); F(currentSum);
  F(powerA); F(powerB); F(powerC); F(powerTotal);
  F(reactiveA); F(reactiveB); F(reactiveC); F(reactiveTotal);
  F(apparentA); F(apparentB); F(apparentC); F(apparentTotal);
  F(pfA); F(pfB); F(pfC); F(pfTotal);
  F(angleA); F(angleB); F(angleC); F(angleTotal); F(frequencyHz);
  F(importKWh); F(exportKWh); F(importKVARh); F(exportKVARh); F(totalKVAh); F(ampHours);
  F(powerDemandW); F(maxPowerDemandW); F(vaDemand); F(maxVaDemand);
  F(neutralDemandA); F(maxNeutralDemandA); F(reactiveDemandVar); F(maxReactiveDemandVar);
  F(thdVoltageA); F(thdVoltageB); F(thdVoltageC);
  F(thdCurrentA); F(thdCurrentB); F(thdCurrentC);
  F(thdVoltageAvg); F(thdCurrentAvg); F(thdVoltageAB); F(thdVoltageBC); F(thdVoltageCA); F(thdVoltageLineAvg);
  F(currentDemandA); F(currentDemandB); F(currentDemandC);
  F(maxCurrentDemandA); F(maxCurrentDemandB); F(maxCurrentDemandC);
  F(totalKWh); F(totalKVARh);
  F(importKWhA); F(importKWhB); F(importKWhC);
  F(exportKWhA); F(exportKWhB); F(exportKWhC);
  F(totalKWhA); F(totalKWhB); F(totalKWhC);
  F(importKVARhA); F(importKVARhB); F(importKVARhC);
  F(exportKVARhA); F(exportKVARhB); F(exportKVARhC);
  F(totalKVARhA); F(totalKVARhB); F(totalKVARhC);
  F(resetTotalKWh); F(resetTotalKVARh); F(resetImportKWh); F(resetExportKWh);
  F(resetImportKVARh); F(resetExportKVARh);
#undef F

  p.addField("online", d.online);
  p.addField("valid", d.valid);
  p.addField("qualityFlags", (uint32_t)d.qualityFlags);
  p.addField("modbusTransactions", (uint32_t)mb.transactions);
  p.addField("modbusSuccessful", (uint32_t)mb.successful);
  p.addField("modbusTimeouts", (uint32_t)mb.timeouts);
  p.addField("modbusCrcErrors", (uint32_t)mb.crcErrors);
  p.addField("modbusFrameErrors", (uint32_t)mb.frameErrors);
  p.addField("modbusExceptionErrors", (uint32_t)mb.exceptionErrors);
  p.addField("modbusRetries", (uint32_t)mb.retries);
  p.addField("modbusLastResponseMs", (uint32_t)mb.lastResponseMs);
}

bool influxSend(const SDM630Data &d, const DeviceStats &sys, const ModbusStats &mb) {
  if (WiFi.status() != WL_CONNECTED) return false;

  Point p(MEASUREMENT_NAME);
  p.addTag("machine", "Fabrica_Principal");
  p.addTag("panel", PANEL_NAME);
  p.addTag("firmware", FIRMWARE_VERSION);
  p.addTag("location", LOCATION_NAME);
  p.addTag("device", DEVICE_NAME);
  addAllFields(p, d, mb);

  if (!influx.writePoint(p)) return false;

  Point e("esp8266");
  e.addTag("device", DEVICE_NAME);
  e.addTag("location", LOCATION_NAME);
  e.addField("alive", true);
  e.addField("uptime", (uint32_t)sys.uptime);
  e.addField("freeHeap", (uint32_t)sys.freeHeap);
  e.addField("wifiRSSI", sys.wifiRSSI);
  e.addField("influxErrors", (uint32_t)sys.influxErrors);
  e.addField("influxWrites", (uint32_t)sys.influxWrites);

  return influx.writePoint(e);
}
