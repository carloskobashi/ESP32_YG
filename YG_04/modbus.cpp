#include "modbus.h"

ModbusRTU::ModbusRTU()
{
    serial = nullptr;

    enablePin = -1;

    crcErrors = 0;
    timeoutErrors = 0;
    frameErrors = 0;
}

//======================================================

bool ModbusRTU::begin(
    HardwareSerial *port,
    uint32_t baudrate,
    int8_t rxPin,
    int8_t txPin,
    int8_t dePin)
{
    serial = port;

    enablePin = dePin;

    serial->begin(
        baudrate,
        SERIAL_8N1,
        rxPin,
        txPin);

    serial->setPins(
        -1,
        -1,
        -1,
        enablePin);

    serial->setMode(
        UART_MODE_RS485_HALF_DUPLEX);

    clearBuffer();

    return true;
}

//======================================================

uint32_t ModbusRTU::getCRCErrorCount()
{
    return crcErrors;
}

uint32_t ModbusRTU::getTimeoutCount()
{
    return timeoutErrors;
}

uint32_t ModbusRTU::getFrameErrorCount()
{
    return frameErrors;
}

//======================================================

void ModbusRTU::clearBuffer()
{
    while (serial->available())
    {
        serial->read();
    }
}

//======================================================

bool ModbusRTU::readHoldingRegisters(
    uint8_t slave,
    uint16_t startRegister,
    uint16_t quantity,
    uint16_t *destination)
{
    return transaction(
        slave,
        0x03,
        startRegister,
        quantity,
        destination);
}

//======================================================

bool ModbusRTU::readInputRegisters(
    uint8_t slave,
    uint16_t startRegister,
    uint16_t quantity,
    uint16_t *destination)
{
    return transaction(
        slave,
        0x04,
        startRegister,
        quantity,
        destination);
}

//======================================================

uint16_t ModbusRTU::crc16(
    const uint8_t *buffer,
    uint16_t length)
{
    uint16_t crc = 0xFFFF;

    while (length--)
    {
        crc ^= *buffer++;

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

//======================================================

bool ModbusRTU::receiveFrame(
    uint8_t *buffer,
    uint16_t &length,
    uint32_t timeout)
{
    length = 0;

    uint32_t start = millis();

    while ((millis() - start) < timeout)
    {
        while (serial->available())
        {
            buffer[length++] = serial->read();

            start = millis();

            if (length >= 256)
            {
                frameErrors++;
                return false;
            }
        }
    }

    if (length < 5)
    {
        timeoutErrors++;
        return false;
    }

    return true;
}

//======================================================

bool ModbusRTU::transaction(
    uint8_t slave,
    uint8_t function,
    uint16_t startRegister,
    uint16_t quantity,
    uint16_t *destination)
{
    uint8_t tx[8];

    tx[0] = slave;
    tx[1] = function;

    tx[2] = startRegister >> 8;
    tx[3] = startRegister & 0xFF;

    tx[4] = quantity >> 8;
    tx[5] = quantity & 0xFF;

    uint16_t crc = crc16(tx, 6);

    tx[6] = crc & 0xFF;
    tx[7] = crc >> 8;

    clearBuffer();

    serial->write(tx, sizeof(tx));

    serial->flush();

    uint8_t rx[256];

    uint16_t rxLength;

    if (!receiveFrame(rx, rxLength, 300))
    {
        return false;
    }

    uint16_t receivedCRC =
        rx[rxLength - 2] |
        (rx[rxLength - 1] << 8);

    uint16_t calculatedCRC =
        crc16(rx, rxLength - 2);

    if (receivedCRC != calculatedCRC)
    {
        crcErrors++;
        return false;
    }

    if (rx[0] != slave)
    {
        frameErrors++;
        return false;
    }

    if (rx[1] != function)
    {
        frameErrors++;
        return false;
    }

    uint8_t bytes = rx[2];

    if (bytes != quantity * 2)
    {
        frameErrors++;
        return false;
    }

    for (uint16_t i = 0; i < quantity; i++)
    {
    destination[i] =
        ((uint16_t)rx[4 + i * 2] << 8) |
        rx[3 + i * 2];
    }

    return true;
}