#include "wifi_manager.h"
#include "config.h"

#include <WiFi.h>

static uint32_t lastReconnect = 0;

//======================================================

bool wifiBegin()
{
    WiFi.mode(WIFI_STA);

    WiFi.setAutoReconnect(true);

    WiFi.persistent(true);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD);

    Serial.println();
    Serial.print("Conectando WiFi");

    uint32_t t = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);

        Serial.print(".");

        if (millis() - t > 20000)
        {
            Serial.println();
            Serial.println("No fue posible conectar.");

            return false;
        }
    }

    Serial.println();

    Serial.println("WiFi conectado");

    Serial.print("IP: ");

    Serial.println(WiFi.localIP());

    return true;
}

//======================================================

void wifiLoop()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    if (millis() - lastReconnect < 5000)
        return;

    lastReconnect = millis();

    Serial.println("Reconectando WiFi...");

    WiFi.disconnect(true);

    delay(100);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD);
}

//======================================================

bool wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}