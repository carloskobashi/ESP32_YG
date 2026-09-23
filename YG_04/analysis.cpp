#include "analysis.h"
#include <math.h>
#include <string.h>
#include "motor_config.h"

#define TREND_SAMPLES 60

static float currentHistory[TREND_SAMPLES];
static float powerHistory[TREND_SAMPLES];
static float pfHistory[TREND_SAMPLES];

static uint8_t trendIndex = 0;

Analysis analysis;

void Analysis::begin()
{
    lastMillis = millis();

    data.starts = 0;
    data.runtimeSeconds = 0;
    data.health = 100.0f;
    strcpy(data.lastEvent, "Sistema iniciado");
    data.totalEvents = 0;
    data.newEvent = true;
    data.maxVoltage = 0;
    data.minVoltage = 1000;
    data.maxCurrent = 0;
    data.maxPower = 0;
    data.maxApparentPower = 0;
    data.maxTemperature = 0;
    data.stoppedSeconds = 0;
    data.idleSeconds = 0;
    data.lowLoadSeconds = 0;
    data.normalLoadSeconds = 0;
    data.highLoadSeconds = 0;
    data.overloadSeconds = 0;
    data.utilization = 0;
    data.severity = 0;
    data.possibleMechanicalProblem = false;
    data.possibleElectricalProblem = false;

    if(data.possibleMechanicalProblem)
    {
        strcpy(data.statusText,"ADVERTENCIA");
        strcpy(data.diagnosticText,"Posible problema mecanico");
        strcpy(data.recommendationText,
            "Revisar rodamientos, rotor y carga");
        data.severity = 1;
    }

    else if(data.possibleElectricalProblem)
    {
        strcpy(data.statusText,"ALARMA");
        strcpy(data.diagnosticText,"Problema electrico");
        strcpy(data.recommendationText,
            "Revisar alimentacion trifasica");
        data.severity = 2;
    }

    else
    {
        strcpy(data.statusText,"NORMAL");
        strcpy(data.diagnosticText,"Operacion correcta");
        strcpy(data.recommendationText,"Sin acciones");
        data.severity = 0;
    }

    data.currentTrend1m = 0;
    data.powerTrend1m = 0;
    data.pfTrend1m = 0;

    data.currentTrendState = 0;
    data.powerTrendState = 0;
    data.pfTrendState = 0;

    memset(currentHistory, 0, sizeof(currentHistory));
    memset(powerHistory, 0, sizeof(powerHistory));
    memset(pfHistory, 0, sizeof(pfHistory));

    data.currentIncreasing = false;
    data.powerIncreasing = false;
    data.healthDecreasing = false;

    data.anomalyCounter = 0;

historyIndex = 0;
historyCount = 0;

}

static void setEvent(AnalysisData &d, const char *txt)
{
    strncpy(d.lastEvent, txt, sizeof(d.lastEvent) - 1);
    d.lastEvent[sizeof(d.lastEvent) - 1] = '\0';

    d.totalEvents++;

    d.newEvent = true;
}

static void setDiagnosis(
    AnalysisData &d,
    uint8_t severity,
    const char *status,
    const char *diag,
    const char *recommendation)
{
    d.severity = severity;

    strncpy(d.statusText, status, sizeof(d.statusText)-1);
    d.statusText[sizeof(d.statusText)-1]=0;

    strncpy(d.diagnosticText, diag, sizeof(d.diagnosticText)-1);
    d.diagnosticText[sizeof(d.diagnosticText)-1]=0;

    strncpy(d.recommendationText,
            recommendation,
            sizeof(d.recommendationText)-1);

    d.recommendationText[
        sizeof(d.recommendationText)-1]=0;
}

void Analysis::update(const PZEMData &m)
{
    //==============================
    // PROMEDIOS
    //==============================

    data.voltageAvg =
        (m.voltageA +
         m.voltageB +
         m.voltageC) / 3.0f;

    data.currentAvg =
        (m.currentA +
         m.currentB +
         m.currentC) / 3.0f;

    data.pfAvg =
        (m.powerFactorA +
         m.powerFactorB +
         m.powerFactorC) / 3.0f;

    data.totalActivePower =
        fabs(m.activePowerA) +
        fabs(m.activePowerB) +
        fabs(m.activePowerC);

    data.totalReactivePower =
        fabs(m.reactivePowerA) +
        fabs(m.reactivePowerB) +
        fabs(m.reactivePowerC);

    data.totalApparentPower =
        m.apparentPowerA +
        m.apparentPowerB +
        m.apparentPowerC;

    data.powerAvg =
        data.totalActivePower / 3.0f;

    static unsigned long trendTimer = millis();

    if(millis() - trendTimer >= 1000)
    {
        trendTimer += 1000;

        currentHistory[trendIndex] = data.currentAvg;
        powerHistory[trendIndex]   = data.powerAvg;
        pfHistory[trendIndex]      = data.pfAvg;

        trendIndex++;

        if(trendIndex >= TREND_SAMPLES)
            trendIndex = 0;
    }

    float currentAverage = 0;
    float powerAverage = 0;
    float pfAverage = 0;

    for(int i=0;i<TREND_SAMPLES;i++)
    {
        currentAverage += currentHistory[i];
        powerAverage += powerHistory[i];
        pfAverage += pfHistory[i];
    }

    currentAverage /= TREND_SAMPLES;
    powerAverage /= TREND_SAMPLES;
    pfAverage /= TREND_SAMPLES;

currentHistory[historyIndex] = data.currentAvg;
powerHistory[historyIndex]   = data.totalActivePower;
pfHistory[historyIndex]      = data.pfAvg;

historyIndex++;

if(historyIndex >= 60)
    historyIndex = 0;

if(historyCount < 60)
    historyCount++;

//float currentAverage = 0;
//float powerAverage = 0;
//float pfAverage = 0;

for(uint8_t i = 0; i < historyCount; i++)
{
    currentAverage += currentHistory[i];
    powerAverage += powerHistory[i];
    pfAverage += pfHistory[i];
}

currentAverage /= historyCount;
powerAverage /= historyCount;
pfAverage /= historyCount;

data.currentTrend1m = currentAverage;
data.powerTrend1m = powerAverage;
data.pfTrend1m = pfAverage;

data.currentTrendState = 0;
if(data.currentAvg > currentAverage * 1.05f)
    data.currentTrendState = 1;
else if(data.currentAvg < currentAverage * 0.95f)
    data.currentTrendState = -1;

data.powerTrendState = 0;
if(data.totalActivePower > powerAverage * 1.05f)
    data.powerTrendState = 1;
else if(data.totalActivePower < powerAverage * 0.95f)
    data.powerTrendState = -1;

data.pfTrendState = 0;
if(data.pfAvg > pfAverage * 1.05f)
    data.pfTrendState = 1;
else if(data.pfAvg < pfAverage * 0.95f)
    data.pfTrendState = -1;

    // Corriente

    if(data.currentAvg > currentAverage * 1.05f)
        data.currentTrendState = 1;
    else if(data.currentAvg < currentAverage * 0.95f)
        data.currentTrendState = -1;
    else
        data.currentTrendState = 0;

    // Potencia

    if(data.totalActivePower  > powerAverage * 1.05f)
        data.powerTrendState = 1;
    else if(data.totalActivePower  < powerAverage * 0.95f)
        data.powerTrendState = -1;
    else
        data.powerTrendState = 0;

    // Factor de potencia

    if(data.pfAvg > pfAverage * 1.05f)
        data.pfTrendState = 1;
    else if(data.pfAvg < pfAverage * 0.95f)
        data.pfTrendState = -1;
    else
        data.pfTrendState = 0;

    //==============================
    // DESBALANCE DE VOLTAJE
    //==============================

    float dv1 = fabs(m.voltageA - data.voltageAvg);
    float dv2 = fabs(m.voltageB - data.voltageAvg);
    float dv3 = fabs(m.voltageC - data.voltageAvg);

    float dv = dv1;

    if (dv2 > dv) dv = dv2;
    if (dv3 > dv) dv = dv3;

    data.voltageUnbalance =
        (dv / data.voltageAvg) * 100.0f;

    //==============================
    // DESBALANCE DE CORRIENTE
    //==============================

    if(data.currentAvg > 0.05f)
    {
        float di1 = fabs(m.currentA - data.currentAvg);
        float di2 = fabs(m.currentB - data.currentAvg);
        float di3 = fabs(m.currentC - data.currentAvg);

        float di = di1;

        if(di2 > di) di = di2;
        if(di3 > di) di = di3;

        data.currentUnbalance =
            (di / data.currentAvg) * 100.0f;
    }
    else
    {
        data.currentUnbalance = 0;
    }

        //=========================================
    // ESTADO DEL MOTOR
    //=========================================

    data.running = data.currentAvg > 0.50f;

    if(data.running && !lastRunning)
        data.starts++;

    lastRunning = data.running;

    unsigned long now = millis();

    if(data.running)
    {
        data.runtimeSeconds += (now - lastMillis) / 1000;
    }

    lastMillis = now;

    //=========================================
    // PORCENTAJE DE CARGA
    // Motor 40 HP
    //=========================================

    float ratedPower =
        MOTOR_HP * 746.0f / MOTOR_EFFICIENCY;



    if(ratedPower > 0)
        data.loadPercent =
                (data.currentAvg / MOTOR_RATED_CURRENT) * 100.0f;
    else
        data.loadPercent = 0;


        //----------------------------------
    // Estadísticas de tiempo
    //----------------------------------

    static unsigned long lastSecond = millis();

    if(millis() - lastSecond >= 1000)
    {
        lastSecond += 1000;

        switch(data.motorState)
        {
            case MOTOR_STOPPED:
                data.stoppedSeconds++;
                break;

            case MOTOR_IDLE:
                data.idleSeconds++;
                break;

            case MOTOR_LOW_LOAD:
                data.lowLoadSeconds++;
                break;

            case MOTOR_NORMAL_LOAD:
                data.normalLoadSeconds++;
                break;

            case MOTOR_HIGH_LOAD:
                data.highLoadSeconds++;
                break;

            case MOTOR_OVERLOAD:
                data.overloadSeconds++;
                break;
        }

        uint32_t total =
            data.stoppedSeconds +
            data.idleSeconds +
            data.lowLoadSeconds +
            data.normalLoadSeconds +
            data.highLoadSeconds +
            data.overloadSeconds;

        if(total > 0)
        {
            data.utilization =
                100.0f *
                (data.normalLoadSeconds +
                 data.highLoadSeconds +
                 data.overloadSeconds)
                / total;
        }
    }

        //=========================================
    // CALCULO D TENDENCIAS
    //=========================================
data.currentIncreasing =
    data.currentTrendState > 0;

data.powerIncreasing =
    data.powerTrendState > 0;

data.healthDecreasing =
    data.health < 80.0f;


            //=========================================
    // anomalía persistente
    //=========================================

if(data.currentIncreasing &&
   data.healthDecreasing)
{
    if(data.anomalyCounter < 60000)
        data.anomalyCounter++;
}
else
{
    data.anomalyCounter = 0;
}

if(data.anomalyCounter > 30)
{
    strcpy(data.statusText,"ADVERTENCIA");

    strcpy(data.diagnosticText,
           "Tendencia de deterioro");

    strcpy(data.recommendationText,
           "Programar inspeccion preventiva");

    data.severity = 1;
}
    //=========================================
    // ALARMAS
    //=========================================

    data.overVoltage =
        (m.voltageA > ALARM_VOLTAGE_HIGH) ||
        (m.voltageB > ALARM_VOLTAGE_HIGH) ||
        (m.voltageC > ALARM_VOLTAGE_HIGH);

    data.underVoltage =
        (m.voltageA < ALARM_VOLTAGE_LOW) ||
        (m.voltageB < ALARM_VOLTAGE_LOW) ||
        (m.voltageC < ALARM_VOLTAGE_LOW);


        if(data.loadPercent < 35.0f)
{
    data.lowPF = false;
}
else
{
    data.lowPF =
        (data.pfAvg < WARNING_PF);
}

    data.phaseLoss =
        data.running &&
        (
            m.currentA < 0.20f ||
            m.currentB < 0.20f ||
            m.currentC < 0.20f
        );

    data.overload =
        data.running &&
        data.loadPercent > ALARM_LOAD;


    data.underload =
        data.running &&
        data.loadPercent < LOAD_LIGHT_MAX;

            //=========================================
    // HEALTH INDEX
    //=========================================

    data.health = 100.0f;

    data.health -= data.voltageUnbalance * 2.0f;
    data.health -= data.currentUnbalance * 1.5f;

    if(data.loadPercent < LOAD_LIGHT_MAX)
    {
        data.lowPF = false;
    }
    else
    {
        data.lowPF =
            (data.pfAvg < WARNING_PF);
    }

    if(data.overVoltage)
        data.health -= 10.0f;

    if(data.underVoltage)
        data.health -= 10.0f;

    if(data.overload)
        data.health -= 20.0f;

    if(data.phaseLoss)
        data.health -= 40.0f;

    if(data.health < 0.0f)
        data.health = 0.0f;

    if(data.health > 100.0f)
        data.health = 100.0f;

    //=========================================
    // ESTADO DEL MOTOR
    //=========================================

    data.motorState = MOTOR_STOPPED;

    if(data.phaseLoss)
    {
        data.motorState = MOTOR_FAULT;
        }
    else if(!data.running)
    {
        data.motorState = MOTOR_STOPPED;
    }
    else if(data.loadPercent < LOAD_IDLE_MAX)
    {
        data.motorState = MOTOR_IDLE;
    }
    else if(data.loadPercent < LOAD_LIGHT_MAX)
    {
        data.motorState = MOTOR_LOW_LOAD;
    }
    else if(data.loadPercent < LOAD_NORMAL_MAX)
    {
        data.motorState = MOTOR_NORMAL_LOAD;
    }
    else if(data.loadPercent < LOAD_HIGH_MAX)
    {
        data.motorState = MOTOR_HIGH_LOAD;
    }
    else
    {
        data.motorState = MOTOR_OVERLOAD;
    }

static uint8_t lastState = 255;

if(lastState != data.motorState)
{
    lastState = data.motorState;

    switch(data.motorState)
    {
        case MOTOR_STOPPED:
            setEvent(data, "Motor detenido");
            break;

        case MOTOR_STARTING:
            setEvent(data, "Motor arrancando");
            break;

        case MOTOR_IDLE:
            setEvent(data, "Motor en vacio");
            break;

        case MOTOR_LOW_LOAD:
            setEvent(data, "Carga baja");
            break;

        case MOTOR_NORMAL_LOAD:
            setEvent(data, "Carga normal");
            break;

        case MOTOR_HIGH_LOAD:
            setEvent(data, "Carga alta");
            break;

        case MOTOR_OVERLOAD:
            setEvent(data, "Sobrecarga");
            break;

        case MOTOR_FAULT:
            setEvent(data, "Falla detectada");
            break;
    }
}

    //----------------------------------
    // MAXIMOS Y MINIMOS
    //----------------------------------

    if(data.voltageAvg > data.maxVoltage)
        data.maxVoltage = data.voltageAvg;

    if(data.voltageAvg < data.minVoltage)
        data.minVoltage = data.voltageAvg;

    if(data.currentAvg > data.maxCurrent)
        data.maxCurrent = data.currentAvg;

    if(data.totalActivePower > data.maxPower)
        data.maxPower = data.totalActivePower;

    if(data.totalApparentPower > data.maxApparentPower)
        data.maxApparentPower = data.totalApparentPower;


//--------------------------------------------------
// Diagnóstico inteligente
//--------------------------------------------------

data.possibleMechanicalProblem = false;
data.possibleElectricalProblem = false;

// Corriente desbalanceada con voltajes correctos
if(data.currentUnbalance > 8.0f &&
   data.voltageUnbalance < 2.5f)
{
    data.possibleMechanicalProblem = true;
}

// Mucho consumo con poca potencia útil
if(data.currentAvg > MOTOR_RATED_CURRENT * 0.80f &&
   data.pfAvg < 0.65f &&
   data.loadPercent < 60.0f)
{
    data.possibleMechanicalProblem = true;
}

// Voltajes desbalanceados
if(data.voltageUnbalance > WARNING_UNBALANCE)
{
    data.possibleElectricalProblem = true;
}

// Bajo voltaje
if(data.underVoltage)
{
    data.possibleElectricalProblem = true;
}

// Sobre voltaje
if(data.overVoltage)
{
    data.possibleElectricalProblem = true;
}

//-------------------------------------------------
// DIAGNOSTICO INTELIGENTE
//-------------------------------------------------

if(!data.running)
{
    setDiagnosis(
        data,
        0,
        "DETENIDO",
        "Motor sin operar",
        "Esperando arranque");
}
else if(data.phaseLoss)
{
    setDiagnosis(
        data,
        3,
        "FALLA",
        "Perdida de fase",
        "Detener equipo y revisar alimentacion");
}
else if(data.overload)
{
    setDiagnosis(
        data,
        3,
        "CRITICO",
        "Motor sobrecargado",
        "Reducir carga inmediatamente");
}
else if(data.overVoltage)
{
    setDiagnosis(
        data,
        2,
        "ALARMA",
        "Sobre voltaje",
        "Revisar alimentacion electrica");
}
else if(data.underVoltage)
{
    setDiagnosis(
        data,
        2,
        "ALARMA",
        "Bajo voltaje",
        "Revisar acometida electrica");
}
else if(data.possibleMechanicalProblem)
{
    setDiagnosis(
        data,
        2,
        "REVISAR",
        "Posible problema mecanico",
        "Revisar rodamientos, bandas o desalineacion");
}
else if(data.possibleElectricalProblem)
{
    setDiagnosis(
        data,
        2,
        "REVISAR",
        "Posible problema electrico",
        "Revisar alimentacion y conexiones");
}
else if(data.lowPF)
{
    setDiagnosis(
        data,
        1,
        "ADVERTENCIA",
        "Factor de potencia bajo",
        "Revisar carga o banco de capacitores");
}
else if(data.underload)
{
    setDiagnosis(
        data,
        1,
        "OPERANDO",
        "Motor con carga ligera",
        "Operacion normal");
}
else
{
    setDiagnosis(
        data,
        0,
        "NORMAL",
        "Operacion correcta",
        "Sin acciones requeridas");
}
      return;
}