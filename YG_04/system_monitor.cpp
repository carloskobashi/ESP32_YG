#include "system_monitor.h"

#include <WiFi.h>
#include <esp_system.h>
#include <esp_spi_flash.h>

SystemMonitor::SystemMonitor()
{
}

bool SystemMonitor::begin()
{
    return true;
}

void SystemMonitor::setLoopTime(uint32_t ms)
{
    _loopTime = ms;
}

void SystemMonitor::addWifiReconnect()
{
    _wifiReconnects++;
}

void SystemMonitor::addInfluxError()
{
    _influxErrors++;
}

void SystemMonitor::update(SystemData &data)
{
    data.clear();

    //------------------------------------------
    // Heartbeat
    //------------------------------------------

    data.alive = true;

    //------------------------------------------
    // Tiempo
    //------------------------------------------

    data.uptime = millis() / 1000;

    //------------------------------------------
    // RAM
    //------------------------------------------

    data.freeHeap = ESP.getFreeHeap();

    data.minFreeHeap = ESP.getMinFreeHeap();

    data.heapSize = ESP.getHeapSize();

    if (data.heapSize > 0)
    {
        data.heapUsage =
            100.0f *
            ((float)(data.heapSize - data.freeHeap) /
             (float)data.heapSize);
    }

    //------------------------------------------
    // CPU
    //------------------------------------------

    data.cpuMHz = ESP.getCpuFreqMHz();

    //------------------------------------------
    // Flash
    //------------------------------------------

    data.flashSize = ESP.getFlashChipSize();

    data.sketchSize = ESP.getSketchSize();

    data.freeSketchSpace = ESP.getFreeSketchSpace();

    //------------------------------------------
    // WIFI
    //------------------------------------------

    data.wifiConnected =
        (WiFi.status() == WL_CONNECTED);

    if (data.wifiConnected)
    {
        data.wifiRSSI = WiFi.RSSI();

        data.wifiChannel = WiFi.channel();

        data.ip = WiFi.localIP().toString();

        data.mac = WiFi.macAddress();
    }

    //------------------------------------------
    // Estadísticas
    //------------------------------------------

    data.loopTime = _loopTime;

    data.wifiReconnects = _wifiReconnects;

    data.influxErrors = _influxErrors;

    //------------------------------------------
    // Reset
    //------------------------------------------

    switch (esp_reset_reason())
    {
        case ESP_RST_POWERON:
            data.resetReason = "POWER_ON";
            break;

        case ESP_RST_SW:
            data.resetReason = "SOFTWARE";
            break;

        case ESP_RST_PANIC:
            data.resetReason = "PANIC";
            break;

        case ESP_RST_INT_WDT:
            data.resetReason = "INT_WDT";
            break;

        case ESP_RST_TASK_WDT:
            data.resetReason = "TASK_WDT";
            break;

        case ESP_RST_WDT:
            data.resetReason = "WDT";
            break;

        case ESP_RST_DEEPSLEEP:
            data.resetReason = "DEEPSLEEP";
            break;

        case ESP_RST_BROWNOUT:
            data.resetReason = "BROWNOUT";
            break;

        default:
            data.resetReason = "UNKNOWN";
            break;
    }
}