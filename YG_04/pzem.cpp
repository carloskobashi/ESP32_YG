#define DEBUG_PZEM
#include "pzem.h"

//======================================================

PZEM::PZEM()
{
}

//======================================================

bool PZEM::begin(
    HardwareSerial *serial,
    uint32_t baudrate,
    int8_t rxPin,
    int8_t txPin,
    int8_t dePin)
{
    return modbus.begin(
        serial,
        baudrate,
        rxPin,
        txPin,
        dePin);
}

//======================================================

bool PZEM::readBlock(
    uint16_t startRegister,
    uint16_t quantity)
{
    return modbus.readInputRegisters(
        PZEM_SLAVE_ID,
        startRegister,
        quantity,
        reg);
}

//======================================================
// 16 bits Unsigned
//======================================================

float PZEM::reg16u(
    uint16_t index,
    float scale)
{
    return ((uint16_t)reg[index]) / scale;
}

//======================================================
// 16 bits Signed
//======================================================

float PZEM::reg16s(
    uint16_t index,
    float scale)
{
    return ((int16_t)reg[index]) / scale;
}

//======================================================
// 32 bits Unsigned
//======================================================

float PZEM::reg32u(
    uint16_t index,
    float scale)
{
    uint32_t value =
        ((uint32_t)reg[index + 1] << 16) |
        reg[index];

    return (float)value / scale;
}

//======================================================
// 32 bits Signed
//======================================================

float PZEM::reg32s(
    uint16_t index,
    float scale)
{
    int32_t value =
        ((int32_t)((int16_t)reg[index + 1]) << 16) |
        reg[index];

    return (float)value / scale;
}

//======================================================

bool PZEM::read(PZEMData &data)
{
    data.clear();

    if (!readBlock(0x0000,64))
    {
        data.online = false;

        data.crcErrors = modbus.getCRCErrorCount();
        data.timeoutErrors = modbus.getTimeoutCount();
        data.frameErrors = modbus.getFrameErrorCount();

        return false;
    }

    data.online = true;

    data.crcErrors = modbus.getCRCErrorCount();
    data.timeoutErrors = modbus.getTimeoutCount();
    data.frameErrors = modbus.getFrameErrorCount();

#ifdef DEBUG_PZEM

    Serial.println();
    Serial.println("========== REGISTROS PZEM ==========");

    for(int i=0;i<64;i++)
    {
        Serial.printf("R%02d = %04X\r\n",i,reg[i]);
    }

    Serial.println("===================================");
    Serial.println();

#endif


    //--------------------------------------------------
    // VOLTAJES
    //--------------------------------------------------

    data.voltageA = reg16u(0,10.0f);
    data.voltageB = reg16u(1,10.0f);
    data.voltageC = reg16u(2,10.0f);

    //--------------------------------------------------
    // CORRIENTES
    //--------------------------------------------------

    data.currentA = reg16u(3,100.0f);
    data.currentB = reg16u(4,100.0f);
    data.currentC = reg16u(5,100.0f);

    //--------------------------------------------------
    // FRECUENCIAS
    //--------------------------------------------------

    data.frequencyA = reg16u(6,100.0f);
    data.frequencyB = reg16u(7,100.0f);
    data.frequencyC = reg16u(8,100.0f);

    //--------------------------------------------------
    // ANGULOS DE VOLTAJE
    //--------------------------------------------------

    data.voltageAngleB = reg16u(9,100.0f);
    data.voltageAngleC = reg16u(10,100.0f);

    //--------------------------------------------------
    // ANGULOS DE CORRIENTE
    //--------------------------------------------------

    data.currentAngleA = reg16u(11,100.0f);
    data.currentAngleB = reg16u(12,100.0f);
    data.currentAngleC = reg16u(13,100.0f);

    //--------------------------------------------------
    // POTENCIA ACTIVA
    //--------------------------------------------------

    data.activePowerA = reg32s(14,10.0f);
    data.activePowerB = reg32s(16,10.0f);
    data.activePowerC = reg32s(18,10.0f);
    data.activePowerTotal = reg32s(32,10.0f);
    data.activePowerCalculated =
        data.activePowerA +
        data.activePowerB +
        data.activePowerC;


    //--------------------------------------------------
    // POTENCIA REACTIVA
    //--------------------------------------------------

    data.reactivePowerA = reg32s(20,10.0f);
    data.reactivePowerB = reg32s(22,10.0f);
    data.reactivePowerC = reg32s(24,10.0f);
    data.reactivePowerTotal = reg32s(34,10.0f);

    //--------------------------------------------------
    // POTENCIA APARENTE
    //--------------------------------------------------

    data.apparentPowerA = reg32s(26,10.0f);
    data.apparentPowerB = reg32s(28,10.0f);
    data.apparentPowerC = reg32s(30,10.0f);
    data.apparentPowerTotal = reg32s(36,10.0f);

    //--------------------------------------------------
    // FACTOR DE POTENCIA
    //--------------------------------------------------

    uint16_t pfAB = reg[38];

    data.powerFactorA = ((pfAB >> 8) & 0xFF) / 100.0f;
    data.powerFactorB = (pfAB & 0xFF) / 100.0f;

    uint16_t pfCT = reg[39];

    data.powerFactorC = ((pfCT >> 8) & 0xFF) / 100.0f;
    data.powerFactorTotal = (pfCT & 0xFF) / 100.0f;

    //--------------------------------------------------
    // ENERGIA ACTIVA
    //--------------------------------------------------

    data.activeEnergyA = reg32u(40,10.0f);
    data.activeEnergyB = reg32u(42,10.0f);
    data.activeEnergyC = reg32u(44,10.0f);
    data.activeEnergyTotal = reg32u(58,10.0f);
    data.activeEnergyCalculated =
        data.activeEnergyA +
        data.activeEnergyB +
        data.activeEnergyC;

    //--------------------------------------------------
    // ENERGIA REACTIVA
    //--------------------------------------------------

    data.reactiveEnergyA = reg32u(46,10.0f);
    data.reactiveEnergyB = reg32u(48,10.0f);
    data.reactiveEnergyC = reg32u(50,10.0f);
    data.reactiveEnergyTotal = reg32u(60,10.0f);

    //--------------------------------------------------
    // ENERGIA APARENTE
    //--------------------------------------------------

    data.apparentEnergyA = reg32u(52,10.0f);
    data.apparentEnergyB = reg32u(54,10.0f);
    data.apparentEnergyC = reg32u(56,10.0f);
    data.apparentEnergyTotal = reg32u(62,10.0f);

        //--------------------------------------------------
    // Validaciones
    //--------------------------------------------------

    if (data.powerFactorA > 1.00f) data.powerFactorA = 1.00f;
    if (data.powerFactorB > 1.00f) data.powerFactorB = 1.00f;
    if (data.powerFactorC > 1.00f) data.powerFactorC = 1.00f;
    if (data.powerFactorTotal > 1.00f) data.powerFactorTotal = 1.00f;

    //--------------------------------------------------
    // Fin lectura correcta
    //--------------------------------------------------

    return true;
}