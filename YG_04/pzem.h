#ifndef PZEM_H
#define PZEM_H

#include <Arduino.h>
#include "modbus.h"

//======================================================
// Dirección Modbus del medidor
//======================================================

#define PZEM_SLAVE_ID 0x01

//======================================================
// Estructura de datos del medidor
//======================================================

struct PZEMData
{
    //--------------------------------------------------
    // Estado
    //--------------------------------------------------

    bool online;

    //--------------------------------------------------
    // Voltajes (V)
    //--------------------------------------------------

    float voltageA;
    float voltageB;
    float voltageC;

    //--------------------------------------------------
    // Corrientes (A)
    //--------------------------------------------------

    float currentA;
    float currentB;
    float currentC;

    //--------------------------------------------------
    // Frecuencia (Hz)
    //--------------------------------------------------

    float frequencyA;
    float frequencyB;
    float frequencyC;

    //--------------------------------------------------
    // Ángulos de Voltaje (°)
    //--------------------------------------------------

    float voltageAngleB;
    float voltageAngleC;

    //--------------------------------------------------
    // Ángulos de Corriente (°)
    //--------------------------------------------------

    float currentAngleA;
    float currentAngleB;
    float currentAngleC;

    //--------------------------------------------------
    // Potencia Activa (W)
    //--------------------------------------------------

    float activePowerA;
    float activePowerB;
    float activePowerC;
    float activePowerTotal;
    float activePowerCalculated;

    //--------------------------------------------------
    // Potencia Reactiva (Var)
    //--------------------------------------------------

    float reactivePowerA;
    float reactivePowerB;
    float reactivePowerC;
    float reactivePowerTotal;

    //--------------------------------------------------
    // Potencia Aparente (VA)
    //--------------------------------------------------

    float apparentPowerA;
    float apparentPowerB;
    float apparentPowerC;
    float apparentPowerTotal;

    //--------------------------------------------------
    // Factor de Potencia
    //--------------------------------------------------

    float powerFactorA;
    float powerFactorB;
    float powerFactorC;
    float powerFactorTotal;

    //--------------------------------------------------
    // Energía Activa (kWh)
    //--------------------------------------------------

    float activeEnergyA;
    float activeEnergyB;
    float activeEnergyC;
    float activeEnergyTotal;
    float activeEnergyCalculated;
    

    //--------------------------------------------------
    // Energía Reactiva (kVarh)
    //--------------------------------------------------

    float reactiveEnergyA;
    float reactiveEnergyB;
    float reactiveEnergyC;
    float reactiveEnergyTotal;

    //--------------------------------------------------
    // Energía Aparente (kVAh)
    //--------------------------------------------------

    float apparentEnergyA;
    float apparentEnergyB;
    float apparentEnergyC;
    float apparentEnergyTotal;

    //--------------------------------------------------
    // Estadísticas de comunicación
    //--------------------------------------------------

    uint32_t crcErrors;
    uint32_t timeoutErrors;
    uint32_t frameErrors;

    //--------------------------------------------------
    // Limpiar estructura
    //--------------------------------------------------

    void clear()
    {
        memset(this, 0, sizeof(PZEMData));
    }
};

//======================================================
// Clase PZEM
//======================================================

class PZEM
{
public:

    PZEM();

    bool begin(
        HardwareSerial *serial,
        uint32_t baudrate,
        int8_t rxPin,
        int8_t txPin,
        int8_t dePin);

    bool read(PZEMData &data);

private:

    ModbusRTU modbus;

    uint16_t reg[128];

    //--------------------------------------------------

    float reg16u(
        uint16_t index,
        float scale);

    float reg16s(
        uint16_t index,
        float scale);

    float reg32u(
        uint16_t index,
        float scale);

    float reg32s(
        uint16_t index,
        float scale);

    //--------------------------------------------------

    bool readBlock(
        uint16_t startRegister,
        uint16_t quantity);

};

#endif