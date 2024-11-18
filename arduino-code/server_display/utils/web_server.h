#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <Arduino.h>

class DisplayWebServer {
private:
    WebServer server;
    bool *refreshRequested;

public:
    DisplayWebServer(bool *refreshFlag) : server(80), refreshRequested(refreshFlag) {
        server.enableCORS(true);
        
        server.on("/refresh", HTTP_POST, [this]() {
            *refreshRequested = true;
            server.send(200, "text/plain", "Refresh requested");
            esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
            if (wakeup_reason == ESP_SLEEP_WAKEUP_WIFI) {
                Serial.println("web_server.h file wakup");
            }
        });

        server.on("/refresh", HTTP_GET, [this]() {
            *refreshRequested = true;
            server.send(200, "text/plain", "Refresh requested");
            esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
            if (wakeup_reason == ESP_SLEEP_WAKEUP_WIFI) {
                Serial.println("web_server.h file wakup");
            }
        });
    }

    void begin() {
        server.begin();
    }

    void handle() {
        server.handleClient();
    }
};

#endif // WEB_SERVER_H