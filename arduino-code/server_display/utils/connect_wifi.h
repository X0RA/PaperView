#ifndef CONNECT_WIFI_H
#define CONNECT_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include "wifi_config.h"
#include <esp_wifi.h>

void printWifiConfig() {
    wifi_config_t wifi_config;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    Serial.printf("WiFi config ssid: %s\n", wifi_config.sta.ssid);
    Serial.printf("WiFi config password: %s\n", wifi_config.sta.password);
    Serial.printf("WiFi config channel: %d\n", wifi_config.sta.channel);
    Serial.printf("WiFi config listen interval: %d\n", wifi_config.sta.listen_interval);
    Serial.printf("WiFi config sort method: %d\n", wifi_config.sta.sort_method);
    Serial.printf("WiFi config threshold: %d\n", wifi_config.sta.threshold);
    Serial.printf("WiFi config pmf cfg: %d\n", wifi_config.sta.pmf_cfg);
}

// Need to work this out when i get a usb voltage displayer
// bool configureWiFi(int listen_interval) {
//     // Create WiFi configuration
//     wifi_config_t wifi_config = {
//         .sta = {
//             .listen_interval = (uint16_t)listen_interval
//         }
//     };
    
//     // Copy SSID and password from your wifi_config.h
//     memcpy(wifi_config.sta.ssid, WIFI_SSID, strlen(WIFI_SSID));
//     memcpy(wifi_config.sta.password, WIFI_PASSWORD, strlen(WIFI_PASSWORD));
    
//     // Set the configuration
//     esp_err_t result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
//     return result == ESP_OK;
// }



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