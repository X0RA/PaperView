#ifndef UTILS_WIFI_H
#define UTILS_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include "../config.h"

String connectedSSID = "";

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool connectToWiFi() {

    for (const WifiNetwork &network : WIFI_NETWORKS) {
        LOG_I("Attempting to connect to WiFi SSID: %s", network.ssid);
        WiFi.begin(network.ssid, network.password);
        int attempts = 0;

        while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS) {
            delay(500);
            LOG_D("Connection attempt %d of %d", attempts + 1, WIFI_MAX_ATTEMPTS);
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            connectedSSID = network.ssid;
            LOG_I("WiFi connected successfully to %s", connectedSSID.c_str());
            LOG_I("IP address: %s", WiFi.localIP().toString().c_str());
            return true;
        }

        LOG_E("Failed to connect to %s after %d attempts", network.ssid, WIFI_MAX_ATTEMPTS);
    }

    LOG_E("WiFi connection failed for all configured networks");
    return false;
}

#endif // UTILS_WIFI_H