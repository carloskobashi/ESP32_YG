# MAQSENSE FABRICA 1.0.0

Firmware para ESP8266 D1 Mini + RS485 + Eastron SDM630MCT.

## Objetivo

- Leer el SDM630MCT por Modbus RTU/RS485.
- Usar Function Code 04 (Input Registers).
- Leer mediciones instantaneas, energia, demanda, THD y energia por fase.
- Validar datos antes de considerarlos confiables.
- Registrar errores Modbus: timeout, CRC, frame, exception y retries.
- Enviar a InfluxDB con tags compatibles con el ecosistema MAQSENSE/YG.
- Mantener OTA y reconexion WiFi sin bloquear el ciclo de adquisicion mas de lo necesario.

## Antes de compilar

Editar `config.h` y colocar:

- WIFI_SSID
- WIFI_PASSWORD
- INFLUXDB_URL
- INFLUXDB_ORG
- INFLUXDB_BUCKET
- INFLUXDB_TOKEN

No se incluyeron credenciales del proyecto YG deliberadamente.

## Hardware provisional

D1 Mini:

- D6/GPIO12 = RX RS485
- D7/GPIO13 = TX RS485
- D5/GPIO14 = DE+RE si el convertidor necesita control manual

El convertidor de la fotografia debe confirmarse como auto-direction o MAX485 manual antes de poner `RS485_AUTO_DIRECTION` en `true` o `false`.

## SDM630MCT

El firmware usa registros de entrada 0x04 y decodifica FLOAT32 IEEE-754 en orden de palabra alto/bajo.

No escribe registros de configuracion del SDM. La version inicial es exclusivamente de lectura.

## Influx

Measurement principal: `fabrica_energia`

Tags:
- machine=Fabrica_Principal
- panel=Principal
- firmware=1.0.0
- location=REPLASTIKA
- device=MAQSENSE-FABRICA

Tambien se genera measurement `esp8266` para diagnostico.

## IMPORTANTE

Esta version es una base de puesta en marcha. No se debe declarar un sistema electrico como "100% confiable" solo por compilar. La puesta en servicio requiere comparar el ESP32/ESP8266 contra el display del SDM630MCT, confirmar CT ratio, tipo de sistema 3P4W y direccion/baud/paridad RS485.
