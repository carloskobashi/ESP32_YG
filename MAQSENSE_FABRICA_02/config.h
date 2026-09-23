#pragma once

// ============================================================
// MAQSENSE - FABRICA - CONFIGURACION
// ============================================================
// Copia este archivo a config_local.h si prefieres separar
// credenciales de la configuracion del proyecto.
// ============================================================

// ---------------- WIFI ----------------
#define WIFI_SSID       "***"
#define WIFI_PASSWORD   "***"

// ---------------- INFLUXDB ----------------
// Usa el mismo servidor que YG. No se modifica el servidor.
#define INFLUXDB_URL    "http://192.168.1.10:8086"
#define INFLUXDB_ORG    "***"
#define INFLUXDB_BUCKET "***"
#define INFLUXDB_TOKEN  "***"

// ---------------- IDENTIDAD ----------------
#define DEVICE_NAME       "MAQSENSE-FABRICA"
#define MEASUREMENT_NAME  "fabrica_energia"
#define LOCATION_NAME     "REPLASTIKA"
#define PANEL_NAME        "Principal"
#define FIRMWARE_VERSION  "1.0.0"

// ---------------- SDM630MCT ----------------
#define SDM_SLAVE_ID      1
#define SDM_BAUD          9600

// ---------------- D1 MINI / RS485 ----------------
// D6 = GPIO12 RX del ESP8266
// D7 = GPIO13 TX del ESP8266
// D5 = GPIO14 DE+RE si el convertidor RS485 requiere direccion manual.
#define RS485_RX_PIN      D7
#define RS485_TX_PIN      D8
#define RS485_DE_RE_PIN   D2

// true  = convertidor con auto-direction (no usa DE/RE)
// false = MAX485/manual: DE y /RE unidos al pin RS485_DE_RE_PIN
#define RS485_AUTO_DIRECTION true

// ---------------- TIEMPOS ----------------
#define METER_READ_INTERVAL_MS     1000UL
#define INFLUX_WRITE_INTERVAL_MS   2000UL
#define WIFI_RETRY_INTERVAL_MS    10000UL
#define WIFI_FULL_RETRY_MS        30000UL
#define MODBUS_TIMEOUT_MS           350UL
#define MODBUS_TURNAROUND_MS         3UL
#define MODBUS_RETRIES                2

// ---------------- DEBUG ----------------
#define SERIAL_BAUD 115200
#define DEBUG_REGISTERS true

// ---------------- VALIDACION ----------------
#define MAX_VOLTAGE_V       700.0f
#define MAX_CURRENT_A     10000.0f
#define MIN_FREQUENCY_HZ     40.0f
#define MAX_FREQUENCY_HZ     70.0f
#define MAX_POWER_W    20000000.0f
#define MAX_ENERGY_KWH 1000000000.0f
#define MAX_THD_PERCENT      500.0f

