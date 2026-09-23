#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

bool wifiBegin();

void wifiLoop();

bool wifiConnected();

#endif