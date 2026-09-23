#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

struct ModbusStats {
  uint32_t transactions = 0;
  uint32_t successful = 0;
  uint32_t timeouts = 0;
  uint32_t crcErrors = 0;
  uint32_t frameErrors = 0;
  uint32_t exceptionErrors = 0;
  uint32_t retries = 0;
  uint32_t lastResponseMs = 0;
};

class SDM630Modbus {
public:
  SDM630Modbus(SoftwareSerial &serial, int8_t deRePin, bool autoDirection);

  void begin(uint32_t baud);
  bool readInputRegisters(uint8_t slave, uint16_t start, uint16_t quantity, uint16_t *dest);
  const ModbusStats &stats() const { return _stats; }
  void resetStats();

private:
  SoftwareSerial &_serial;
  int8_t _deRePin;
  bool _autoDirection;
  ModbusStats _stats;

  static uint16_t crc16(const uint8_t *data, uint16_t len);
  void setTransmit(bool tx);
  bool readExact(uint8_t *buf, size_t len, uint32_t timeoutMs);
  bool transaction(uint8_t slave, uint8_t function, uint16_t start, uint16_t quantity, uint16_t *dest);
};
