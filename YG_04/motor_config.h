#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

//--------------------------------------------------
// DATOS DEL MOTOR
//--------------------------------------------------

#define MOTOR_NAME             "Motor Principal"

#define MOTOR_HP               40.0f

#define MOTOR_EFFICIENCY       0.93f

#define MOTOR_NOMINAL_PF       0.895f

#define MOTOR_SERVICE_FACTOR   1.15f

#define MOTOR_VOLTAGE          460.0f

#define MOTOR_FREQUENCY        60.0f

#define MOTOR_RATED_CURRENT    52.0f

//--------------------------------------------------
// ZONAS DE CARGA
//--------------------------------------------------

#define LOAD_IDLE_MAX          10.0f

#define LOAD_LIGHT_MAX         20.0f
//#define LOAD_LIGHT_MAX         30.0f

#define LOAD_NORMAL_MAX        80.0f

#define LOAD_HIGH_MAX          100.0f

//--------------------------------------------------
// ALARMAS
//--------------------------------------------------

#define WARNING_PF             0.80f

#define WARNING_UNBALANCE      3.0f

#define WARNING_LOAD           85.0f

#define ALARM_LOAD             100.0f

#define ALARM_VOLTAGE_HIGH     265.0f

#define ALARM_VOLTAGE_LOW      230.0f

#endif