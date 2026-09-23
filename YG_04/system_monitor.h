#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>

struct SystemData
{
    // Tiempo
    uint32_t uptime;

    // Memoria
    uint32_t freeHeap;
    uint32_t minFreeHeap;
    uint32_t heapSize;
    float heapUsage;

    // CPU
    uint32_t cpuMHz;

    // Flash
    uint32_t flashSize;
    uint32_t sketchSize;
    uint32_t freeSketchSpace;

    // WiFi
    bool wifiConnected;
    int wifiRSSI;
    uint8_t wifiChannel;

    String ip;
    String mac;

    // Diagnóstico
    String resetReason;

    // Rendimiento
    uint32_t loopTime;

    // Comunicación
    uint32_t wifiReconnects;
    uint32_t influxErrors;

    // Heartbeat
    bool alive;

    void clear()
    {
        uptime = 0;

        freeHeap = 0;
        minFreeHeap = 0;
        heapSize = 0;
        heapUsage = 0;

        cpuMHz = 0;

        flashSize = 0;
        sketchSize = 0;
        freeSketchSpace = 0;

        wifiConnected = false;
        wifiRSSI = 0;
        wifiChannel = 0;

        ip = "";
        mac = "";

        resetReason = "";

        loopTime = 0;

        wifiReconnects = 0;
        influxErrors = 0;

        alive = false;
    }
};

class SystemMonitor
{
public:

    SystemMonitor();

    bool begin();

    void update(SystemData &data);

    void setLoopTime(uint32_t ms);

    void addWifiReconnect();

    void addInfluxError();

private:

    uint32_t _loopTime = 0;
    uint32_t _wifiReconnects = 0;
    uint32_t _influxErrors = 0;
};

#endif