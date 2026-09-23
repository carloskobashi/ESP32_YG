#ifndef CONFIG_H
#define CONFIG_H

//======================================================
// WIFI
//======================================================

#define WIFI_SSID      "***"
#define WIFI_PASSWORD  "***"

//======================================================
// INFLUXDB
//======================================================

#define INFLUXDB_URL      "***"
#define INFLUXDB_TOKEN    "***"
#define INFLUXDB_ORG      "***"
#define INFLUXDB_BUCKET   "***"

//======================================================
// MODBUS / RS485
//======================================================

#define MODBUS_BAUD       9600
#define MODBUS_ID         0x01

// Pines ESP32-S3-RS485-CAN Waveshare
#define RS485_TX_PIN      17
#define RS485_RX_PIN      18
#define RS485_EN_PIN      21

//======================================================
// TIEMPOS
//======================================================

#define READ_INTERVAL     1000      // ms entre lecturas
#define MODBUS_TIMEOUT    300       // ms timeout respuesta


//======================================================

#endif
