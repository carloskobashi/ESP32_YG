#include "influx_manager.h"
#include "config.h"
#include "analysis.h"

#include <WiFi.h>
#include <InfluxDbClient.h>

InfluxDBClient influx(
    INFLUXDB_URL,
    INFLUXDB_ORG,
    INFLUXDB_BUCKET,
    INFLUXDB_TOKEN);

bool influxBegin()
{
    Serial.println("Inicializando InfluxDB...");

    influx.setConnectionParamsV1(
        INFLUXDB_URL,
        INFLUXDB_BUCKET,
        INFLUXDB_ORG,
        INFLUXDB_TOKEN);

    if (influx.validateConnection())
    {
        Serial.println("InfluxDB OK");
        return true;
    }

    Serial.println(influx.getLastErrorMessage());

    return false;
}

bool influxSend(
    const PZEMData &pzem,
    const SystemData &sys)
{
    //----------------------------------------
    // MEDICION CENTRIFUGA
    //----------------------------------------

    Point pzemPoint("centrifuga");

    pzemPoint.addTag("machine", "Centrifuga_40HP");
    pzemPoint.addTag("panel", "Principal");
    pzemPoint.addTag("firmware", "PZEM_gpt_14");
    pzemPoint.addTag("location", "REPLASTIKA");
    pzemPoint.addTag("device","ESP32-S3");

    pzemPoint.addField("online",pzem.online);

    pzemPoint.addField("voltageA",pzem.voltageA);
    pzemPoint.addField("voltageB",pzem.voltageB);
    pzemPoint.addField("voltageC",pzem.voltageC);

    pzemPoint.addField("currentA",pzem.currentA);
    pzemPoint.addField("currentB",pzem.currentB);
    pzemPoint.addField("currentC",pzem.currentC);

    pzemPoint.addField("frequencyA", pzem.frequencyA);
    pzemPoint.addField("frequencyB", pzem.frequencyB);
    pzemPoint.addField("frequencyC", pzem.frequencyC);

    pzemPoint.addField("voltageAngleB", pzem.voltageAngleB);
    pzemPoint.addField("voltageAngleC", pzem.voltageAngleC);

    pzemPoint.addField("currentAngleA", pzem.currentAngleA);
    pzemPoint.addField("currentAngleB", pzem.currentAngleB);
    pzemPoint.addField("currentAngleC", pzem.currentAngleC);

    pzemPoint.addField("activePowerA", pzem.activePowerA);
    pzemPoint.addField("activePowerB", pzem.activePowerB);
    pzemPoint.addField("activePowerC", pzem.activePowerC);
    pzemPoint.addField("activePowerTotal", pzem.activePowerTotal);
    pzemPoint.addField("activePowerCalculated", pzem.activePowerCalculated);

    pzemPoint.addField("reactivePowerA",pzem.reactivePowerA);
    pzemPoint.addField("reactivePowerB",pzem.reactivePowerB);
    pzemPoint.addField("reactivePowerC",pzem.reactivePowerC);
    pzemPoint.addField("reactivePowerTotal", pzem.reactivePowerTotal);

    pzemPoint.addField("apparentPowerA",pzem.apparentPowerA);
    pzemPoint.addField("apparentPowerB",pzem.apparentPowerB);
    pzemPoint.addField("apparentPowerC",pzem.apparentPowerC);
    pzemPoint.addField("apparentPowerTotal", pzem.apparentPowerTotal);

    pzemPoint.addField("powerFactorA", pzem.powerFactorA);
    pzemPoint.addField("powerFactorB", pzem.powerFactorB);
    pzemPoint.addField("powerFactorC", pzem.powerFactorC);
    pzemPoint.addField("powerFactorTotal", pzem.powerFactorTotal);

    //---------------------
    // Active Energy
    //---------------------

    pzemPoint.addField("activeEnergyA", pzem.activeEnergyA);
    pzemPoint.addField("activeEnergyB", pzem.activeEnergyB);
    pzemPoint.addField("activeEnergyC", pzem.activeEnergyC);
    pzemPoint.addField("activeEnergyTotal", pzem.activeEnergyTotal);
    pzemPoint.addField("activeEnergyCalculated", pzem.activeEnergyCalculated);

    //---------------------
    // Reactive Energy
    //---------------------

    pzemPoint.addField("reactiveEnergyA", pzem.reactiveEnergyA);
    pzemPoint.addField("reactiveEnergyB", pzem.reactiveEnergyB);
    pzemPoint.addField("reactiveEnergyC", pzem.reactiveEnergyC);
    pzemPoint.addField("reactiveEnergyTotal", pzem.reactiveEnergyTotal);

    //---------------------
    // Apparent Energy
    //---------------------

    pzemPoint.addField("apparentEnergyA", pzem.apparentEnergyA);
    pzemPoint.addField("apparentEnergyB", pzem.apparentEnergyB);
    pzemPoint.addField("apparentEnergyC", pzem.apparentEnergyC);
    pzemPoint.addField("apparentEnergyTotal", pzem.apparentEnergyTotal);
    pzemPoint.addField("crcErrors", (int)pzem.crcErrors);
    pzemPoint.addField("timeoutErrors", (int)pzem.timeoutErrors);
    pzemPoint.addField("frameErrors", (int)pzem.frameErrors);

    //----------------------------------------
    // ANALISIS
    //----------------------------------------

    pzemPoint.addField("voltage_avg", analysis.data.voltageAvg);
    pzemPoint.addField("current_avg", analysis.data.currentAvg);
    pzemPoint.addField("power_total", analysis.data.totalActivePower);
    pzemPoint.addField("power_apparent_total", analysis.data.totalApparentPower);
    pzemPoint.addField("power_reactive_total", analysis.data.totalReactivePower);
    pzemPoint.addField("pf_avg", analysis.data.pfAvg);
    pzemPoint.addField("voltage_unbalance", analysis.data.voltageUnbalance);
    pzemPoint.addField("current_unbalance", analysis.data.currentUnbalance);
    pzemPoint.addField("motor_load", analysis.data.loadPercent);
    pzemPoint.addField("motor_health", analysis.data.health);
    pzemPoint.addField("motor_running", analysis.data.running);
    pzemPoint.addField("motor_overload", analysis.data.overload);
    pzemPoint.addField("motor_underload", analysis.data.underload);
    pzemPoint.addField("motor_phase_loss", analysis.data.phaseLoss);
    pzemPoint.addField("motor_low_pf", analysis.data.lowPF);
    pzemPoint.addField("motor_over_voltage", analysis.data.overVoltage);
    pzemPoint.addField("motor_under_voltage", analysis.data.underVoltage);
    pzemPoint.addField("motor_starts", (int32_t)analysis.data.starts);
    pzemPoint.addField("runtime_seconds", (int32_t)analysis.data.runtimeSeconds);
    

//------------- Tendencias -------------

    pzemPoint.addField("currentTrend1m", analysis.data.currentTrend1m);
    pzemPoint.addField("powerTrend1m", analysis.data.powerTrend1m);
    pzemPoint.addField("pfTrend1m", analysis.data.pfTrend1m);
    pzemPoint.addField("currentTrendState", analysis.data.currentTrendState);
    pzemPoint.addField("powerTrendState", analysis.data.powerTrendState);
    pzemPoint.addField("pfTrendState", analysis.data.pfTrendState);

//------------- Records -------------

    pzemPoint.addField("maxVoltage", analysis.data.maxVoltage);
    pzemPoint.addField("minVoltage", analysis.data.minVoltage);
    pzemPoint.addField("maxCurrent", analysis.data.maxCurrent);
    pzemPoint.addField("maxPower", analysis.data.maxPower);
    pzemPoint.addField("maxApparentPower", analysis.data.maxApparentPower);

//------------- Estadísticas -------------

    pzemPoint.addField("stoppedSeconds", analysis.data.stoppedSeconds);
    pzemPoint.addField("idleSeconds", analysis.data.idleSeconds);
    pzemPoint.addField("lowLoadSeconds", analysis.data.lowLoadSeconds);
    pzemPoint.addField("normalLoadSeconds", analysis.data.normalLoadSeconds);
    pzemPoint.addField("highLoadSeconds", analysis.data.highLoadSeconds);
    pzemPoint.addField("overloadSeconds", analysis.data.overloadSeconds);
    pzemPoint.addField("utilization", analysis.data.utilization);


//------------- Diagnóstico -------------

    pzemPoint.addField("severity",
                    (int)analysis.data.severity);

    pzemPoint.addField("statusText",
                    analysis.data.statusText);

    pzemPoint.addField("diagnosticText",
                    analysis.data.diagnosticText);

    pzemPoint.addField("recommendationText",
                    analysis.data.recommendationText);

    pzemPoint.addField("possibleMechanicalProblem",
                    analysis.data.possibleMechanicalProblem);

    pzemPoint.addField("possibleElectricalProblem",
                    analysis.data.possibleElectricalProblem);

    pzemPoint.addField("totalEvents",
                    (int32_t)analysis.data.totalEvents);

    pzemPoint.addField("motorState",
                    (int)analysis.data.motorState);


    if(!influx.writePoint(pzemPoint))
    {
        Serial.println(influx.getLastErrorMessage());
        return false;
    }                   

    //----------------------------------------
    // MEDICION ESP32
    //----------------------------------------

    Point espPoint("esp32");

    espPoint.addTag("device","ESP32-S3");
    espPoint.addField("alive",sys.alive);
    espPoint.addField("uptime",sys.uptime);
    espPoint.addField("freeHeap",(int)sys.freeHeap);
    espPoint.addField("minFreeHeap",(int)sys.minFreeHeap);
    espPoint.addField("heapSize",(int)sys.heapSize);
    espPoint.addField("heapUsage",sys.heapUsage);
    espPoint.addField("cpuMHz",(int)sys.cpuMHz);
    espPoint.addField("flashSize",(int)sys.flashSize);
    espPoint.addField("sketchSize",(int)sys.sketchSize);
    espPoint.addField("freeSketchSpace",(int)sys.freeSketchSpace);
    espPoint.addField("wifiConnected",sys.wifiConnected);
    espPoint.addField("wifiRSSI",sys.wifiRSSI);
    espPoint.addField("wifiChannel",(int)sys.wifiChannel);
    espPoint.addField("loopTime",(int)sys.loopTime);
    espPoint.addField("wifiReconnects",(int)sys.wifiReconnects);
    espPoint.addField("influxErrors",(int)sys.influxErrors);
    espPoint.addField("ip",sys.ip);
    espPoint.addField("mac",sys.mac);
    espPoint.addField("resetReason",sys.resetReason);

    if(!influx.writePoint(espPoint))
    {
        Serial.println(influx.getLastErrorMessage());
        return false;
    }

    return true;
}