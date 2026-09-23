#include "modbus_sdm630.h"
#include "config.h"

SDM630Modbus::SDM630Modbus(SoftwareSerial &serial, int8_t deRePin, bool autoDirection)
  : _serial(serial), _deRePin(deRePin), _autoDirection(autoDirection) {}

void SDM630Modbus::begin(uint32_t baud) {
  if (!_autoDirection && _deRePin >= 0) {
    pinMode(_deRePin, OUTPUT);
    digitalWrite(_deRePin, LOW);
  }
  _serial.begin(baud);
  delay(50);
  while (_serial.available()) _serial.read();
}

void SDM630Modbus::setTransmit(bool tx) {
  if (_autoDirection || _deRePin < 0) return;
  digitalWrite(_deRePin, tx ? HIGH : LOW);
  if (tx) delayMicroseconds(150);
}

uint16_t SDM630Modbus::crc16(const uint8_t *data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; ++i)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  }
  return crc;
}

bool SDM630Modbus::readExact(uint8_t *buf, size_t len, uint32_t timeoutMs) {
  size_t got = 0;
  uint32_t deadline = millis() + timeoutMs;
  while (got < len) {
    while (_serial.available() && got < len) buf[got++] = (uint8_t)_serial.read();
    if ((int32_t)(millis() - deadline) >= 0) return false;
    yield();
  }
  return true;
}

bool SDM630Modbus::readInputRegisters(uint8_t slave, uint16_t start, uint16_t quantity, uint16_t *dest) {
  return transaction(slave, 0x04, start, quantity, dest);
}

bool SDM630Modbus::transaction(uint8_t slave, uint8_t function, uint16_t start, uint16_t quantity, uint16_t *dest) {
  if (quantity == 0 || quantity > 80) return false;

  for (uint8_t attempt = 0; attempt <= MODBUS_RETRIES; ++attempt) {
    if (attempt) _stats.retries++;
    _stats.transactions++;

    while (_serial.available()) _serial.read();

    uint8_t tx[8];
    tx[0] = slave;
    tx[1] = function;
    tx[2] = highByte(start);
    tx[3] = lowByte(start);
    tx[4] = highByte(quantity);
    tx[5] = lowByte(quantity);
    uint16_t crc = crc16(tx, 6);
    tx[6] = lowByte(crc);
    tx[7] = highByte(crc);

    setTransmit(true);
    uint32_t t0 = millis();
    _serial.write(tx, sizeof(tx));
    _serial.flush();
    setTransmit(false);
    delay(MODBUS_TURNAROUND_MS);

    uint8_t header[3];
    if (!readExact(header, 3, MODBUS_TIMEOUT_MS)) {
      _stats.timeouts++;
      continue;
    }

    if (header[0] != slave) {
      _stats.frameErrors++;
      continue;
    }

    // Exception response: function | 0x80 + exception code + CRC.
    if (header[1] & 0x80) {
      uint8_t tail[2];
      if (readExact(tail, 2, MODBUS_TIMEOUT_MS)) _stats.exceptionErrors++;
      else _stats.timeouts++;
      continue;
    }

    if (header[1] != function) {
      _stats.frameErrors++;
      continue;
    }

    uint8_t byteCount = header[2];
    uint16_t expected = quantity * 2;
    if (byteCount != expected || expected > 160) {
      _stats.frameErrors++;
      continue;
    }

    uint8_t payload[162];
    payload[0] = header[0];
    payload[1] = header[1];
    payload[2] = header[2];

    if (!readExact(payload + 3, expected + 2, MODBUS_TIMEOUT_MS)) {
      _stats.timeouts++;
      continue;
    }

    uint16_t received = payload[3 + expected] | ((uint16_t)payload[4 + expected] << 8);
    uint16_t calculated = crc16(payload, 3 + expected);
    if (received != calculated) {
      _stats.crcErrors++;
      continue;
    }

    for (uint16_t i = 0; i < quantity; ++i)
      dest[i] = ((uint16_t)payload[3 + i * 2] << 8) | payload[4 + i * 2];

    _stats.successful++;
    _stats.lastResponseMs = millis() - t0;
    return true;
  }

  return false;
}

void SDM630Modbus::resetStats() { _stats = ModbusStats(); }
