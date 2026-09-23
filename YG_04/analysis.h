#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "pzem.h"

enum
{
    MOTOR_STOPPED = 0,
    MOTOR_STARTING,
    MOTOR_IDLE,
    MOTOR_LOW_LOAD,
    MOTOR_NORMAL_LOAD,
    MOTOR_HIGH_LOAD,
    MOTOR_OVERLOAD,
    MOTOR_FAULT
};

struct AnalysisData
{
    // Promedios
    float voltageAvg;
    float currentAvg;
    float powerAvg;
    float pfAvg;

    // Totales
    float totalActivePower;
    float totalReactivePower;
    float totalApparentPower;

    // Balance
    float voltageUnbalance;
    float currentUnbalance;

    // Motor
    float loadPercent;

    // Estado
    bool running;
    bool overload;
    bool underload;
    bool phaseLoss;
    bool lowPF;
    bool overVoltage;
    bool underVoltage;

    // Estadísticas
    uint32_t starts;
    uint32_t runtimeSeconds;

    // Salud
    float health;
    uint8_t motorState;
    char lastEvent[40];
    uint32_t totalEvents;

    bool newEvent;

    float maxVoltage;
    float minVoltage;
    float maxCurrent;
    float maxPower;
    float maxApparentPower;
    float maxTemperature;

        // Estadísticas de operación

    uint32_t stoppedSeconds;
    uint32_t idleSeconds;
    uint32_t lowLoadSeconds;
    uint32_t normalLoadSeconds;
    uint32_t highLoadSeconds;
    uint32_t overloadSeconds;

    float utilization;

    uint8_t severity;

    char statusText[32];

    char diagnosticText[64];

    char recommendationText[80];

    bool possibleMechanicalProblem;
    bool possibleElectricalProblem;

    float currentTrend1m;
    float powerTrend1m;
    float pfTrend1m;

    int8_t currentTrendState;
    int8_t powerTrendState;
    int8_t pfTrendState;

    bool currentIncreasing;
    bool powerIncreasing;
    bool healthDecreasing;

uint16_t anomalyCounter;
};

class Analysis
{
public:

    void begin();

    void update(const PZEMData &m);

    AnalysisData data;

private:

    bool lastRunning=false;

    unsigned long lastMillis=0;

    float currentHistory[60];
    float powerHistory[60];
    float pfHistory[60];

    uint8_t historyIndex = 0;
    uint8_t historyCount = 0;
};

extern Analysis analysis;

#endif