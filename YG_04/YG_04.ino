#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ModbusMaster.h>
#include <InfluxDbClient.h>
#include <InfluxDBCloud.h>
#include <math.h>

#include "config.h"


// ============================================================
// MAQSENSE v3
// MAQUINAS - DATOS - ANALISIS
//
// YG194E-9SY
// ESP32-S3-RS485-CAN Waveshare
//
// FUNCIONES:
//
//   YG194E-9SY
//        |
//        v
//      RS485
//        |
//        v
//      ESP32
//        |
//        +----> InfluxDB
//        |
//        +----> ArduinoOTA
//
// ============================================================


// ============================================================
// VERSION
// ============================================================

const char* MAQSENSE_VERSION = "3.1.2";


// ============================================================
// MAQUINA
// ============================================================

const char* MACHINE_NAME = "Centrifuga_40HP";

const char* MEASUREMENT_NAME = "centrifuga";


// ============================================================
// UMBRALES
// ============================================================
//
// PROVISIONALES.
//
// Los validaremos durante la operación real.
//
// ============================================================

const float UMBRAL_SIN_CARGA  = 1.0;
const float UMBRAL_PRODUCCION = 11.9;


// ============================================================
// PRODUCCION
// ============================================================

const float KG_POR_MINUTO = 5.0;

const float PRECIO_KWH = 3.5;

const float VALOR_KG = 1.5;


// ============================================================
// INTERVALOS
// ============================================================

// Lectura YG
const unsigned long INTERVALO_LECTURA = 2000;

// Envío Influx
const unsigned long INTERVALO_INFLUX = 5000;


// ============================================================
// OTA
// ============================================================

const char* OTA_HOSTNAME = "MAQSENSE-YG";
const char* OTA_PASSWORD = "maqsense";


// ============================================================
// MODBUS
// ============================================================

ModbusMaster node;

HardwareSerial RS485Serial(1);


// ============================================================
// INFLUXDB
// ============================================================

InfluxDBClient influxClient(
  INFLUXDB_URL,
  INFLUXDB_ORG,
  INFLUXDB_BUCKET,
  INFLUXDB_TOKEN
);

Point influxPoint(
  MEASUREMENT_NAME
);


// ============================================================
// REGISTROS YG
// ============================================================

#define FIRST_REGISTER 9
#define LAST_REGISTER 78

#define TOTAL_REGISTERS \
  (LAST_REGISTER - FIRST_REGISTER + 1)

uint16_t values[TOTAL_REGISTERS];


// ============================================================
// CONTROL
// ============================================================

unsigned long ultimoMonitor = 0;

unsigned long ultimoInflux = 0;

unsigned long lecturasOK = 0;

unsigned long erroresModbus = 0;

unsigned long erroresInflux = 0;

unsigned long enviosInflux = 0;


// ============================================================
// ESTADO YG
// ============================================================

bool ygOnline = false;

bool influxOnline = false;

bool monitorAutomaticoActivo = true;

bool wifiEstabaConectado = false;
bool otaInicializado = false;
unsigned long ultimoIntentoWiFi = 0;
unsigned long ultimoBeginWiFi = 0;
const unsigned long INTERVALO_RECONEXION_WIFI = 5000;
const unsigned long INTERVALO_REINICIO_WIFI = 15000;


// ============================================================
// VARIABLES ELECTRICAS
// ============================================================

float voltageA = 0.0;
float voltageB = 0.0;
float voltageC = 0.0;

float currentA = 0.0;
float currentB = 0.0;
float currentC = 0.0;

float currentTotal = 0.0;

float powerKW = 0.0;

float reactiveKVAR = 0.0;

float apparentKVA = 0.0;

float powerFactor = 0.0;

float frequencyHz = 0.0;

float energyKWh = 0.0;

float reactiveEnergyKVARh = 0.0;


// ============================================================
// ACUMULADOS AUXILIARES
// ============================================================

float acumuladoAux1 = 0.0;

float acumuladoAux2 = 0.0;


// ============================================================
// PF POR FASE
// ============================================================

float pfA = 0.0;

float pfB = 0.0;

float pfC = 0.0;


// ============================================================
// ESTADO DE MAQUINA
// ============================================================

enum EstadoMaquina
{
  ESTADO_SIN_DATOS,
  ESTADO_DETENIDA,
  ESTADO_ENCENDIDA_SIN_CARGA,
  ESTADO_PRODUCIENDO
};

EstadoMaquina estadoActual =
    ESTADO_SIN_DATOS;


// ============================================================
// ENERGIA DESDE ARRANQUE
// ============================================================

float energiaInicialKWh = 0.0;

bool energiaInicialValida = false;


// ============================================================
// RS485
// ============================================================

void preTransmission()
{
  digitalWrite(
    RS485_EN_PIN,
    HIGH
  );
}


void postTransmission()
{
  digitalWrite(
    RS485_EN_PIN,
    LOW
  );
}


// ============================================================
// FLOAT MODBUS
// ============================================================

float leerFloat(int index)
{
  if (
    index < 0 ||
    index + 1 >= TOTAL_REGISTERS
  )
  {
    return NAN;
  }


  uint32_t raw =
      ((uint32_t)values[index] << 16) |
      values[index + 1];


  float resultado;

  memcpy(
    &resultado,
    &raw,
    sizeof(resultado)
  );


  return resultado;
}


// ============================================================
// LEER BLOQUE MODBUS
// ============================================================

bool leerBloque(
  uint16_t startRegister,
  uint16_t cantidad
)
{
  uint8_t resultado =
      node.readHoldingRegisters(
        startRegister,
        cantidad
      );


  if (
    resultado !=
    node.ku8MBSuccess
  )
  {
    erroresModbus++;

    ygOnline = false;

    return false;
  }


  for (
    uint16_t i = 0;
    i < cantidad;
    i++
  )
  {
    int index =
        (startRegister - FIRST_REGISTER) + i;


    if (
      index >= 0 &&
      index < TOTAL_REGISTERS
    )
    {
      values[index] =
          node.getResponseBuffer(i);
    }
  }


  return true;
}


// ============================================================
// LEER YG COMPLETO
// ============================================================

bool leerRegistros()
{
  bool ok = true;


  if (!leerBloque(9, 20))
    ok = false;


  if (!leerBloque(29, 20))
    ok = false;


  if (!leerBloque(49, 20))
    ok = false;


  if (!leerBloque(69, 10))
    ok = false;


  ygOnline = ok;


  if (ok)
  {
    lecturasOK++;
  }


  return ok;
}


// ============================================================
// INTERPRETAR YG
// ============================================================

void interpretarDatos()
{
  // ----------------------------------------------------------
  // VOLTAJES
  // ----------------------------------------------------------

  voltageA =
      leerFloat(0) / 1000.0;

  voltageB =
      leerFloat(2) / 1000.0;

  voltageC =
      leerFloat(4) / 1000.0;


  // ----------------------------------------------------------
  // CORRIENTES
  // ----------------------------------------------------------

  currentA =
      leerFloat(6);

  currentB =
      leerFloat(8);

  currentC =
      leerFloat(10);

  currentTotal =
      leerFloat(12);


  // ----------------------------------------------------------
  // POTENCIA ACTIVA
  // ----------------------------------------------------------

  powerKW =
      leerFloat(14) / 1000000.0;


  // ----------------------------------------------------------
  // POTENCIA REACTIVA
  // ----------------------------------------------------------

  reactiveKVAR =
      leerFloat(16) / 1000000.0;


  // ----------------------------------------------------------
  // FACTOR DE POTENCIA
  // ----------------------------------------------------------

  powerFactor =
      leerFloat(18);


  // ----------------------------------------------------------
  // FRECUENCIA
  // ----------------------------------------------------------

  frequencyHz =
      leerFloat(20);


  // ----------------------------------------------------------
  // ENERGIA ACTIVA
  // ----------------------------------------------------------

  energyKWh =
      leerFloat(22);


  // ----------------------------------------------------------
  // ACUMULADO AUXILIAR 1
  // ----------------------------------------------------------

  acumuladoAux1 =
      leerFloat(24);


  // ----------------------------------------------------------
  // ENERGIA REACTIVA
  // ----------------------------------------------------------

  reactiveEnergyKVARh =
      leerFloat(26);


  // ----------------------------------------------------------
  // ACUMULADO AUXILIAR 2
  // ----------------------------------------------------------

  acumuladoAux2 =
      leerFloat(28);


  // ----------------------------------------------------------
  // POTENCIA APARENTE
  // ----------------------------------------------------------

  apparentKVA =
      sqrt(
        (powerKW * powerKW) +
        (reactiveKVAR * reactiveKVAR)
      );


  // ----------------------------------------------------------
  // PF POR FASE
  // ----------------------------------------------------------

  pfA =
      leerFloat(50);

  pfB =
      leerFloat(52);

  pfC =
      leerFloat(54);
}


// ============================================================
// DETERMINAR ESTADO
// ============================================================

void determinarEstado()
{
  // ----------------------------------------------------------
  // SIN DATOS
  // ----------------------------------------------------------

  if (!ygOnline)
  {
    estadoActual =
        ESTADO_SIN_DATOS;

    return;
  }


  // ----------------------------------------------------------
  // DETENIDA
  // ----------------------------------------------------------

  if (
    fabs(currentTotal) <
      UMBRAL_SIN_CARGA
    &&
    fabs(powerKW) < 0.05
  )
  {
    estadoActual =
        ESTADO_DETENIDA;

    return;
  }


  // ----------------------------------------------------------
  // PRODUCIENDO
  // ----------------------------------------------------------

  if (
    currentTotal >=
    UMBRAL_PRODUCCION
  )
  {
    estadoActual =
        ESTADO_PRODUCIENDO;

    return;
  }


  // ----------------------------------------------------------
  // ENCENDIDA SIN CARGA
  // ----------------------------------------------------------

  if (
    currentTotal >=
    UMBRAL_SIN_CARGA
  )
  {
    estadoActual =
        ESTADO_ENCENDIDA_SIN_CARGA;

    return;
  }


  estadoActual =
      ESTADO_DETENIDA;
}


// ============================================================
// TEXTO ESTADO
// ============================================================

const char* obtenerTextoEstado()
{
  switch (estadoActual)
  {
    case ESTADO_SIN_DATOS:
      return "SIN_DATOS";

    case ESTADO_DETENIDA:
      return "DETENIDA";

    case ESTADO_ENCENDIDA_SIN_CARGA:
      return "ENCENDIDA_SIN_CARGA";

    case ESTADO_PRODUCIENDO:
      return "PRODUCIENDO";
  }


  return "DESCONOCIDO";
}


// ============================================================
// ENERGIA DEL PERIODO
// ============================================================

float obtenerEnergiaPeriodo()
{
  if (!energiaInicialValida)
    return 0.0;


  float delta =
      energyKWh -
      energiaInicialKWh;


  if (delta < 0)
    delta = 0;


  return delta;
}


// ============================================================
// COSTO ENERGIA
// ============================================================

float obtenerCostoEnergia()
{
  return
      obtenerEnergiaPeriodo() *
      PRECIO_KWH;
}


// ============================================================
// CONECTAR WIFI
// ============================================================

bool conectarWiFi()
{
  Serial.println();

  Serial.println(
    "Conectando WiFi..."
  );


  WiFi.mode(
    WIFI_STA
  );

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  WiFi.setHostname(
    OTA_HOSTNAME
  );


  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  unsigned long inicio =
      millis();


  while (
    WiFi.status() !=
      WL_CONNECTED
    &&
    millis() - inicio <
      15000
  )
  {
    delay(500);

    Serial.print(".");
  }


  Serial.println();


  if (
    WiFi.status() !=
    WL_CONNECTED
  )
  {
    Serial.println(
      "WiFi NO conectado."
    );

    return false;
  }


  Serial.println(
    "WiFi conectado."
  );


  Serial.print(
    "IP: "
  );

  Serial.println(
    WiFi.localIP()
  );


  Serial.print(
    "RSSI: "
  );

  Serial.print(
    WiFi.RSSI()
  );

  Serial.println(
    " dBm"
  );

  wifiEstabaConectado = true;
  ultimoIntentoWiFi = millis();
  ultimoBeginWiFi = millis();

  return true;
}


// ============================================================
// CONFIGURAR OTA
// ============================================================

void configurarOTA()
{
  if (otaInicializado)
  {
    return;
  }

  Serial.println();

  Serial.println(
    "Inicializando OTA..."
  );


  ArduinoOTA.setHostname(
    OTA_HOSTNAME
  );


  ArduinoOTA.setPort(
    3232
  );


  ArduinoOTA.onStart([]()
  {
    Serial.println();

    Serial.println(
      "=========================================="
    );

    Serial.println(
      "OTA: INICIANDO ACTUALIZACION"
    );

    Serial.println(
      "=========================================="
    );
  });


  ArduinoOTA.onEnd([]()
  {
    Serial.println();

    Serial.println(
      "OTA: ACTUALIZACION TERMINADA"
    );
  });


  ArduinoOTA.onProgress(
    [](unsigned int progress,
       unsigned int total)
    {
      static int ultimo = -1;


      int porcentaje =
          (progress * 100) /
          total;


      if (
        porcentaje !=
        ultimo
      )
      {
        ultimo =
            porcentaje;


        Serial.printf(
          "OTA: %u%%\n",
          porcentaje
        );
      }
    }
  );


  ArduinoOTA.onError(
    [](ota_error_t error)
    {
      Serial.printf(
        "OTA ERROR [%u]\n",
        error
      );
    }
  );


  ArduinoOTA.begin();
  otaInicializado = true;


  Serial.println(
    "OTA listo."
  );


  Serial.print(
    "Hostname OTA: "
  );

  Serial.println(
    OTA_HOSTNAME
  );


  Serial.println(
    "Puerto OTA: 3232"
  );
}


// ============================================================
// CONECTAR INFLUXDB
// ============================================================

bool conectarInflux()
{
  Serial.println();

  Serial.println(
    "Probando conexion con InfluxDB..."
  );


  if (
    influxClient.validateConnection()
  )
  {
    Serial.println(
      "InfluxDB conectado."
    );

    influxOnline = true;

    return true;
  }


  Serial.print(
    "InfluxDB ERROR: "
  );

  Serial.println(
    influxClient.getLastErrorMessage()
  );


  influxOnline = false;

  return false;
}


// ============================================================
// PREPARAR PUNTO INFLUX
// ============================================================

void prepararPoint()
{
  // ----------------------------------------------------------
  // Crear nuevo punto
  // ----------------------------------------------------------

  influxPoint.clearFields();

  influxPoint.clearTags();


  // ----------------------------------------------------------
  // TAGS
  // ----------------------------------------------------------

  influxPoint.addTag(
    "machine",
    MACHINE_NAME
  );


  influxPoint.addTag(
    "device",
    "MAQSENSE-YG"
  );


  // ----------------------------------------------------------
  // ESTADO
  // ----------------------------------------------------------

  influxPoint.addField(
    "estado",
    obtenerTextoEstado()
  );


  // ----------------------------------------------------------
  // VOLTAJES
  // ----------------------------------------------------------

  influxPoint.addField(
    "voltage_a",
    voltageA
  );

  influxPoint.addField(
    "voltage_b",
    voltageB
  );

  influxPoint.addField(
    "voltage_c",
    voltageC
  );


  // ----------------------------------------------------------
  // CORRIENTES
  // ----------------------------------------------------------

  influxPoint.addField(
    "current_a",
    currentA
  );

  influxPoint.addField(
    "current_b",
    currentB
  );

  influxPoint.addField(
    "current_c",
    currentC
  );

  influxPoint.addField(
    "current_total",
    currentTotal
  );


  // ----------------------------------------------------------
  // POTENCIAS
  // ----------------------------------------------------------

  influxPoint.addField(
    "power_kw",
    powerKW
  );

  influxPoint.addField(
    "reactive_kvar",
    reactiveKVAR
  );

  influxPoint.addField(
    "apparent_kva",
    apparentKVA
  );


  // ----------------------------------------------------------
  // FACTOR DE POTENCIA
  // ----------------------------------------------------------

  influxPoint.addField(
    "power_factor",
    powerFactor
  );


  influxPoint.addField(
    "pf_a",
    pfA
  );

  influxPoint.addField(
    "pf_b",
    pfB
  );

  influxPoint.addField(
    "pf_c",
    pfC
  );


  // ----------------------------------------------------------
  // FRECUENCIA
  // ----------------------------------------------------------

  influxPoint.addField(
    "frequency_hz",
    frequencyHz
  );


  // ----------------------------------------------------------
  // ENERGIA ACUMULADA
  // ----------------------------------------------------------

  influxPoint.addField(
    "energy_kwh",
    energyKWh
  );


  influxPoint.addField(
    "reactive_energy_kvarh",
    reactiveEnergyKVARh
  );


  // ----------------------------------------------------------
  // ACUMULADOS AUXILIARES
  // ----------------------------------------------------------

  influxPoint.addField(
    "accum_aux_1",
    acumuladoAux1
  );


  influxPoint.addField(
    "accum_aux_2",
    acumuladoAux2
  );


  // ----------------------------------------------------------
  // ENERGIA DESDE ARRANQUE ESP32
  // ----------------------------------------------------------

  influxPoint.addField(
    "energy_delta_kwh",
    obtenerEnergiaPeriodo()
  );


  influxPoint.addField(
    "energy_cost_mxn",
    obtenerCostoEnergia()
  );


  // ----------------------------------------------------------
  // PARAMETROS MAQSENSE
  // ----------------------------------------------------------

  influxPoint.addField(
    "kg_per_minute",
    KG_POR_MINUTO
  );


  influxPoint.addField(
    "production_threshold_a",
    UMBRAL_PRODUCCION
  );


  influxPoint.addField(
    "no_load_threshold_a",
    UMBRAL_SIN_CARGA
  );
}


// ============================================================
// ENVIAR A INFLUXDB
// ============================================================

bool enviarInflux()
{
  if (
    WiFi.status() !=
    WL_CONNECTED
  )
  {
    influxOnline = false;

    return false;
  }


  prepararPoint();


  if (
    influxClient.writePoint(
      influxPoint
    )
  )
  {
    influxOnline = true;

    enviosInflux++;

    return true;
  }


  erroresInflux++;

  influxOnline = false;


  Serial.print(
    "ERROR INFLUXDB: "
  );

  Serial.println(
    influxClient.getLastErrorMessage()
  );


  return false;
}


// ============================================================
// MOSTRAR ESTADO RESUMIDO
// ============================================================

void mostrarResumen()
{
  interpretarDatos();

  determinarEstado();


  Serial.println();

  Serial.println(
    "--------------------------------------------------"
  );


  Serial.print(
    "Estado: "
  );

  Serial.print(
    obtenerTextoEstado()
  );


  Serial.print(
    " | VA: "
  );

  Serial.print(
    voltageA,
    1
  );

  Serial.print(
    " V"
  );


  Serial.print(
    " | VB: "
  );

  Serial.print(
    voltageB,
    1
  );

  Serial.print(
    " V"
  );


  Serial.print(
    " | VC: "
  );

  Serial.print(
    voltageC,
    1
  );

  Serial.print(
    " V"
  );


  Serial.print(
    " | I: "
  );

  Serial.print(
    currentTotal,
    2
  );

  Serial.print(
    " A"
  );


  Serial.print(
    " | P: "
  );

  Serial.print(
    powerKW,
    3
  );

  Serial.print(
    " kW"
  );


  Serial.print(
    " | Q: "
  );

  Serial.print(
    reactiveKVAR,
    3
  );

  Serial.print(
    " kVAR"
  );


  Serial.print(
    " | PF: "
  );

  Serial.print(
    powerFactor,
    3
  );


  Serial.print(
    " | F: "
  );

  Serial.print(
    frequencyHz,
    2
  );

  Serial.print(
    " Hz"
  );


  Serial.print(
    " | kWh: "
  );

  Serial.print(
    energyKWh,
    3
  );


  Serial.println();


  Serial.print(
    "InfluxDB: "
  );

  Serial.print(
    influxOnline
      ? "ONLINE"
      : "OFFLINE"
  );


  Serial.print(
    " | Envios: "
  );

  Serial.print(
    enviosInflux
  );


  Serial.print(
    " | Errores: "
  );

  Serial.println(
    erroresInflux
  );


  Serial.println(
    "--------------------------------------------------"
  );
}


// ============================================================
// LECTURA COMPLETA
// ============================================================

void lecturaForzada()
{
  Serial.println();

  Serial.println(
    "LECTURA FORZADA"
  );


  if (
    leerRegistros()
  )
  {
    interpretarDatos();

    determinarEstado();

    mostrarResumen();
  }
  else
  {
    Serial.println(
      "ERROR: no se pudo leer YG."
    );
  }
}


// ============================================================
// PRUEBA T
// ============================================================

void pruebaT()
{
  Serial.println();

  Serial.println(
    "--------------------------------------------------"
  );

  Serial.println(
    "PRUEBA T - REGISTROS R0-R9"
  );

  Serial.println(
    "--------------------------------------------------"
  );


  uint8_t resultado =
      node.readHoldingRegisters(
        0,
        10
      );


  if (
    resultado !=
    node.ku8MBSuccess
  )
  {
    Serial.print(
      "ERROR MODBUS = 0x"
    );

    Serial.println(
      resultado,
      HEX
    );

    return;
  }


  Serial.println(
    "COMUNICACION OK"
  );


  for (
    int i = 0;
    i < 10;
    i++
  )
  {
    Serial.print("R");

    if (i < 10)
      Serial.print("0");

    Serial.print(i);

    Serial.print(" = ");

    Serial.println(
      node.getResponseBuffer(i)
    );
  }


  Serial.println(
    "--------------------------------------------------"
  );
}


// ============================================================
// AYUDA
// ============================================================

void mostrarAyuda()
{
  Serial.println();

  Serial.println(
    "=================================================="
  );

  Serial.println(
    "             MAQSENSE v3 - COMANDOS"
  );

  Serial.println(
    "=================================================="
  );

  Serial.println();

  Serial.println(
    "T = Prueba comunicacion R0-R9"
  );

  Serial.println(
    "F = Lectura completa"
  );

  Serial.println(
    "A = Monitor automatico"
  );

  Serial.println(
    "H = Mostrar ayuda"
  );

  Serial.println();

  Serial.println(
    "=================================================="
  );
}


// ============================================================
// MONITOR AUTOMATICO
// ============================================================

void monitorAutomatico()
{
  // ----------------------------------------------------------
  // LECTURA YG
  // ----------------------------------------------------------

  if (
    millis() -
    ultimoMonitor >=
    INTERVALO_LECTURA
  )
  {
    ultimoMonitor = millis();

    if (leerRegistros())
    {
      interpretarDatos();
      determinarEstado();

      if (!energiaInicialValida)
      {
        energiaInicialKWh = energyKWh;
        energiaInicialValida = true;
      }

      mostrarResumen();
    }
    else
    {
      Serial.println();
      Serial.println("ERROR COMUNICACION YG");
    }
  }

  // ----------------------------------------------------------
  // INFLUXDB
  // ----------------------------------------------------------

  if (
    millis() -
    ultimoInflux >=
    INTERVALO_INFLUX
  )
  {
    ultimoInflux = millis();

    if (ygOnline)
    {
      if (enviarInflux())
      {
        Serial.println("  -> InfluxDB: OK");
      }
      else
      {
        Serial.println("  -> InfluxDB: ERROR");
      }
    }
  }
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(
    115200
  );


  delay(1000);


  Serial.println();

  Serial.println();

  Serial.println(
    "=================================================="
  );

  Serial.println(
    "                 MAQSENSE"
  );

  Serial.println(
    "         MAQUINAS - DATOS - ANALISIS"
  );

  Serial.println(
    "=================================================="
  );


  Serial.print(
    "Firmware: MAQSENSE v"
  );

  Serial.println(
    MAQSENSE_VERSION
  );


  // ----------------------------------------------------------
  // RS485
  // ----------------------------------------------------------

  pinMode(
    RS485_EN_PIN,
    OUTPUT
  );


  digitalWrite(
    RS485_EN_PIN,
    LOW
  );


  RS485Serial.begin(
    MODBUS_BAUD,
    SERIAL_8N1,
    RS485_RX_PIN,
    RS485_TX_PIN
  );


  // ----------------------------------------------------------
  // MODBUS
  // ----------------------------------------------------------

  node.begin(
    MODBUS_ID,
    RS485Serial
  );


  node.preTransmission(
    preTransmission
  );


  node.postTransmission(
    postTransmission
  );


  Serial.println();

  Serial.println(
    "CONFIGURACION YG"
  );


  Serial.print(
    "  Slave ID : "
  );

  Serial.println(
    MODBUS_ID
  );


  Serial.print(
    "  Baud     : "
  );

  Serial.println(
    MODBUS_BAUD
  );


  Serial.print(
    "  TX       : GPIO "
  );

  Serial.println(
    RS485_TX_PIN
  );


  Serial.print(
    "  RX       : GPIO "
  );

  Serial.println(
    RS485_RX_PIN
  );


  Serial.print(
    "  EN       : GPIO "
  );

  Serial.println(
    RS485_EN_PIN
  );


  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  if (
    conectarWiFi()
  )
  {
    configurarOTA();

    conectarInflux();
  }
  else
  {
    Serial.println();

    Serial.println(
      "OTA e InfluxDB no disponibles."
    );

    Serial.println(
      "El YG continuara funcionando."
    );
  }


  // ----------------------------------------------------------
  // AYUDA
  // ----------------------------------------------------------

  mostrarAyuda();


  // ----------------------------------------------------------
  // PRIMERA LECTURA
  // ----------------------------------------------------------

  delay(1000);


  Serial.println();

  Serial.println(
    "Intentando primera lectura del YG..."
  );


  if (
    leerRegistros()
  )
  {
    interpretarDatos();

    determinarEstado();


    energiaInicialKWh =
        energyKWh;

    energiaInicialValida =
        true;


    mostrarResumen();
  }
  else
  {
    Serial.println(
      "No se pudo realizar primera lectura."
    );
  }

  // ----------------------------------------------------------
  // MONITOR AUTOMATICO DESDE ARRANQUE
  // ----------------------------------------------------------

  ultimoMonitor = millis();
  ultimoInflux = millis();
  monitorAutomaticoActivo = true;

  Serial.println();
  Serial.println("MONITOR AUTOMATICO ACTIVO DESDE ARRANQUE.");
  Serial.println("Lectura YG cada 2 segundos.");
  Serial.println("InfluxDB cada 5 segundos.");
  Serial.println("No es necesario enviar el comando A.");
}


// ============================================================
// MANTENER WIFI
// ============================================================
//
// Objetivo:
// 1. Recuperar WiFi automaticamente sin reiniciar el ESP32.
// 2. Mantener el YG funcionando durante la perdida de red.
// 3. Recuperar InfluxDB al volver la red.
// 4. Recuperar OTA si el ESP32 inicio sin WiFi.
//
// Estrategia:
// - Cada 5 s: WiFi.reconnect().
// - Cada 15 s sin recuperacion: reinicio completo de la
//   interfaz WiFi y nuevo WiFi.begin().
//
// NO reinicia el ESP32.
// ============================================================

void iniciarWiFiDesdeCero()
{
  Serial.println();
  Serial.println("Forzando nueva asociacion WiFi...");

  // Desconectar sin borrar credenciales.
  WiFi.disconnect(false, false);

  delay(100);

  // Apagar y volver a levantar la interfaz WiFi.
  WiFi.mode(WIFI_OFF);

  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(OTA_HOSTNAME);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );
}


void mantenerWiFi()
{
  unsigned long ahora = millis();

  // ----------------------------------------------------------
  // WIFI CONECTADO
  // ----------------------------------------------------------

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!wifiEstabaConectado)
    {
      wifiEstabaConectado = true;

      Serial.println();
      Serial.println("==========================================");
      Serial.println("WiFi RECUPERADO.");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      Serial.println("==========================================");

      // Si OTA no estaba activo porque el equipo arranco sin red.
      if (!otaInicializado)
      {
        configurarOTA();
      }

      // Validar nuevamente InfluxDB.
      conectarInflux();
    }

    // Conectado: mantener temporizadores sincronizados.
    ultimoIntentoWiFi = ahora;
    ultimoBeginWiFi = ahora;

    return;
  }

  // ----------------------------------------------------------
  // WIFI DESCONECTADO
  // ----------------------------------------------------------

  if (wifiEstabaConectado)
  {
    wifiEstabaConectado = false;
    influxOnline = false;

    Serial.println();
    Serial.println("==========================================");
    Serial.println("WiFi PERDIDO.");
    Serial.println("El YG continuara funcionando sin WiFi.");
    Serial.println("==========================================");
  }

  // ----------------------------------------------------------
  // PRIMER NIVEL: RECONNECT NORMAL
  // ----------------------------------------------------------

  if (ahora - ultimoIntentoWiFi >= INTERVALO_RECONEXION_WIFI)
  {
    ultimoIntentoWiFi = ahora;

    Serial.println();
    Serial.print("Intentando recuperar WiFi... Estado=");
    Serial.println(WiFi.status());

    bool resultado = WiFi.reconnect();

    Serial.print("WiFi.reconnect(): ");
    Serial.println(resultado ? "OK" : "NO INICIADO");
  }

  // ----------------------------------------------------------
  // SEGUNDO NIVEL: REINICIO COMPLETO DE LA INTERFAZ WIFI
  // ----------------------------------------------------------

  if (ahora - ultimoBeginWiFi >= INTERVALO_REINICIO_WIFI)
  {
    ultimoBeginWiFi = ahora;

    Serial.println();
    Serial.println("WiFi no recuperado en 15 segundos.");

    iniciarWiFiDesdeCero();
  }
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  mantenerWiFi();


  // ----------------------------------------------------------
  // OTA
  // ----------------------------------------------------------

  if (otaInicializado)
  {
    ArduinoOTA.handle();
  }


  // ----------------------------------------------------------
  // MONITOR AUTOMATICO
  // ----------------------------------------------------------

  if (monitorAutomaticoActivo)
  {
    monitorAutomatico();
  }


  // ----------------------------------------------------------
  // COMANDOS
  // ----------------------------------------------------------

  if (
    Serial.available()
  )
  {
    char comando =
        Serial.read();


    if (
      comando >= 'a' &&
      comando <= 'z'
    )
    {
      comando -= 32;
    }


    switch (comando)
    {
      case 'T':

        pruebaT();

        break;

      case 'F':
        lecturaForzada();

        break;

      case 'A':
        monitorAutomaticoActivo = true;
        ultimoMonitor = millis();
        ultimoInflux = millis();

        Serial.println();
        Serial.println("MONITOR AUTOMATICO ACTIVADO.");

        break;

      case 'H':
        mostrarAyuda();

        break;

      case '\n':
      case '\r':
        break;

      default:
        Serial.print(
          "Comando desconocido: "
        );

        Serial.println(
          comando
        );

        Serial.println(
          "Presiona H para ayuda."
        );

        break;
    }
  }


  delay(10);
}

