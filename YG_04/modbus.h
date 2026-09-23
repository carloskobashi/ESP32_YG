#ifndef MODBUS_H
#define MODBUS_H

#include <Arduino.h>

//======================================================
// Clase Modbus RTU para ESP32-S3 RS485 Waveshare
//======================================================

class ModbusRTU
{
public:

    ModbusRTU();

    bool begin(
        HardwareSerial *port,
        uint32_t baudrate,
        int8_t rxPin,
        int8_t txPin,
        int8_t dePin);

    //--------------------------------------------------
    // Función 0x03
    //--------------------------------------------------

    bool readHoldingRegisters(
        uint8_t slave,
        uint16_t startRegister,
        uint16_t quantity,
        uint16_t *destination);

    //--------------------------------------------------
    // Función 0x04
    //--------------------------------------------------

    bool readInputRegisters(
        uint8_t slave,
        uint16_t startRegister,
        uint16_t quantity,
        uint16_t *destination);

    //--------------------------------------------------
    // Estado
    //--------------------------------------------------

    uint32_t getCRCErrorCount();

    uint32_t getTimeoutCount();

    uint32_t getFrameErrorCount();

private:

    HardwareSerial *serial;

    int8_t enablePin;

    uint32_t crcErrors;

    uint32_t timeoutErrors;

    uint32_t frameErrors;

    //--------------------------------------------------

    bool transaction(
        uint8_t slave,
        uint8_t function,
        uint16_t startRegister,
        uint16_t quantity,
        uint16_t *destination);

    //--------------------------------------------------

    uint16_t crc16(
        const uint8_t *buffer,
        uint16_t length);

    //--------------------------------------------------

    void clearBuffer();

    bool receiveFrame(
        uint8_t *buffer,
        uint16_t &length,
        uint32_t timeout);

};

#endif