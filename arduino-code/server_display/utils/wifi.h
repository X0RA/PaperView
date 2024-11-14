#ifndef UTILS_WIFI_H
#define UTILS_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include "../config.h"

bool connectToWiFi()
{
    LOG_I("Attempting to connect to WiFi SSID: %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS)
    {
        delay(500);
        LOG_D("Connection attempt %d of %d", attempts + 1, WIFI_MAX_ATTEMPTS);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        LOG_I("WiFi connected successfully");
        LOG_I("IP address: %s", WiFi.localIP().toString().c_str());
        return true;
    }
    else
    {
        LOG_E("WiFi connection failed after %d attempts", WIFI_MAX_ATTEMPTS);
        return false;
    }
}

#endif // UTILS_WIFI_H