#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>
#include <time.h>

#include "config.h"
#include "modbus_sdm630.h"
#include "sdm630.h"
#include "influx_manager.h"

SoftwareSerial rs485Serial(RS485_RX_PIN, RS485_TX_PIN); // RX, TX
SDM630Modbus modbus(rs485Serial, RS485_DE_RE_PIN, RS485_AUTO_DIRECTION);
SDM630 meter(modbus);
SDM630Data meterData;
DeviceStats deviceStats;

bool otaReady = false;
volatile bool otaInProgress = false;
bool influxOnline = false;
uint32_t lastWiFiAttempt = 0;
uint32_t lastFullWiFiAttempt = 0;
uint32_t lastRead = 0;
uint32_t lastWrite = 0;
uint32_t lastPrint = 0;
uint32_t consecutiveBadReads = 0;

static void printHeader() {
  Serial.println();
  Serial.println(F("============================================================"));
  Serial.println(F(" MAQSENSE - ENERGIA DE FABRICA"));
  Serial.println(F(" SDM630MCT / ESP8266 D1 MINI / RS485 / INFLUXDB"));
  Serial.println(F("============================================================"));
  Serial.print(F("Firmware: ")); Serial.println(FIRMWARE_VERSION);
  Serial.print(F("Modbus ID: ")); Serial.println(SDM_SLAVE_ID);
  Serial.print(F("Baud: ")); Serial.println(SDM_BAUD);
  Serial.print(F("RS485 RX: D6 | TX: D7 | DE/RE: ")); Serial.println(RS485_AUTO_DIRECTION ? F("AUTO") : F("D5"));
}

static void startOTA() {
  if (otaReady || WiFi.status() != WL_CONNECTED) return;

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPort(OTA_PORT);

#if OTA_USE_PASSWORD
  ArduinoOTA.setPassword(OTA_PASSWORD);
#endif

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    Serial.println(F("OTA: inicio - adquisicion pausada"));
  });

  ArduinoOTA.onEnd([]() {
    Serial.println(F("OTA: terminado - reiniciando"));
    otaInProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    static int last = -1;
    int pct = (t > 0) ? (int)((p * 100UL) / t) : 0;
    if (pct != last) {
      last = pct;
      Serial.printf("OTA: %d%%\n", pct);
    }
    yield();
  });

  ArduinoOTA.onError([](ota_error_t e) {
    otaInProgress = false;
    Serial.printf("OTA ERROR: %u\n", e);
  });

  ArduinoOTA.begin();
  otaReady = true;

  Serial.print(F("OTA listo: "));
  Serial.print(OTA_HOSTNAME);
  Serial.print(F(" ("));
  Serial.print(WiFi.localIP());
  Serial.println(F(")"));
}

static bool connectWiFiBlocking(uint32_t timeoutMs = 15000) {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(DEVICE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
    delay(100);
    yield();
  }
  return WiFi.status() == WL_CONNECTED;
}

static void maintainWiFi() {
  uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED) {
    if (!otaReady) startOTA();
    return;
  }

  if (now - lastWiFiAttempt >= WIFI_RETRY_INTERVAL_MS) {
    lastWiFiAttempt = now;
    WiFi.reconnect();
  }

  if (now - lastFullWiFiAttempt >= WIFI_FULL_RETRY_MS) {
    lastFullWiFiAttempt = now;
    WiFi.disconnect();
    delay(50);
    connectWiFiBlocking(8000);
    if (WiFi.status() == WL_CONNECTED) {
      startOTA();
      influxOnline = influxBegin();
    }
  }
}

static void printMeter() {
  Serial.println();
  Serial.println(F("---------------- MEDIDOR ----------------"));
  Serial.printf("Online=%s Valid=%s Quality=0x%08lX\n", meterData.online ? "YES":"NO", meterData.valid ? "YES":"NO", (unsigned long)meterData.qualityFlags);
  Serial.printf("V: %.1f / %.1f / %.1f V\n", meterData.voltageA, meterData.voltageB, meterData.voltageC);
  Serial.printf("I: %.2f / %.2f / %.2f A | IN %.2f A\n", meterData.currentA, meterData.currentB, meterData.currentC, meterData.currentNeutral);
  Serial.printf("P: %.1f / %.1f / %.1f = %.1f W\n", meterData.powerA, meterData.powerB, meterData.powerC, meterData.powerTotal);
  Serial.printf("Q: %.1f VAr | S: %.1f VA | PF: %.3f | Hz: %.2f\n", meterData.reactiveTotal, meterData.apparentTotal, meterData.pfTotal, meterData.frequencyHz);
  Serial.printf("E import: %.3f kWh | export: %.3f kWh | total: %.3f kWh\n", meterData.importKWh, meterData.exportKWh, meterData.totalKWh);
  Serial.printf("THD-I: %.2f / %.2f / %.2f %%\n", meterData.thdCurrentA, meterData.thdCurrentB, meterData.thdCurrentC);
  Serial.printf("Demand P: %.1f W | Max: %.1f W\n", meterData.powerDemandW, meterData.maxPowerDemandW);
  Serial.printf("Modbus: ok=%lu timeout=%lu crc=%lu frame=%lu retries=%lu last=%lums\n",
    (unsigned long)modbus.stats().successful,
    (unsigned long)modbus.stats().timeouts,
    (unsigned long)modbus.stats().crcErrors,
    (unsigned long)modbus.stats().frameErrors,
    (unsigned long)modbus.stats().retries,
    (unsigned long)modbus.stats().lastResponseMs);
  Serial.println(F("------------------------------------------"));
}

static void readMeter() {
  if (meter.read(meterData)) {
    consecutiveBadReads = 0;
  } else {
    consecutiveBadReads++;
    meterData.online = false;
    meterData.valid = false;
  }
}

static void sendInflux() {
  if (WiFi.status() != WL_CONNECTED || !meterData.online) return;

  deviceStats.uptime = millis() / 1000UL;
  deviceStats.freeHeap = ESP.getFreeHeap();
  deviceStats.wifiRSSI = WiFi.RSSI();
  deviceStats.influxErrors = 0;
  deviceStats.influxWrites++;

  if (!influxSend(meterData, deviceStats, modbus.stats())) {
    deviceStats.influxErrors++;
    influxOnline = false;
  } else {
    influxOnline = true;
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  printHeader();

  modbus.begin(SDM_BAUD);

  Serial.println(F("Conectando WiFi..."));
  if (connectWiFiBlocking()) {
    Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
    Serial.print(F("RSSI: ")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
    startOTA();
    influxOnline = influxBegin();
    Serial.printf("InfluxDB: %s\n", influxOnline ? "OK" : "ERROR");
  } else {
    Serial.println(F("WiFi no disponible; el medidor seguira trabajando localmente."));
  }

  readMeter();
  printMeter();
  lastRead = millis();
  lastWrite = millis();
  lastPrint = millis();
}

void loop() {
  uint32_t now = millis();

  maintainWiFi();

  // ArduinoOTA debe atenderse continuamente.
  if (otaReady) ArduinoOTA.handle();

  // Durante una actualización no iniciamos nuevas transacciones
  // Modbus ni escrituras a Influx.
  if (otaInProgress) {
    delay(1);
    yield();
    return;
  }

  if (now - lastRead >= METER_READ_INTERVAL_MS) {
    lastRead = now;
    readMeter();
  }

  if (now - lastWrite >= INFLUX_WRITE_INTERVAL_MS) {
    lastWrite = now;
    sendInflux();
  }

  if (now - lastPrint >= 5000UL) {
    lastPrint = now;
    printMeter();
  }

  delay(2);
}
