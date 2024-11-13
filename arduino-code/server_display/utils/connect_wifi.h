#ifndef CONNECT_WIFI_H
#define CONNECT_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include "wifi_config.h"

String connectToWiFi()
{
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());

        String ip_str = "IP: " + WiFi.localIP().toString();
        return ip_str;
    }
    else
    {
        Serial.println("\nWiFi connection failed!");
        return "Failed...";
    }
}

#endif // CONNECT_WIFI_H