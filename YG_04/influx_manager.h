#ifndef INFLUX_MANAGER_H
#define INFLUX_MANAGER_H

#include <Arduino.h>
#include "pzem.h"
#include "system_monitor.h"

bool influxBegin();

bool influxSend(
    const PZEMData &pzem,
    const SystemData &sys);

#endif