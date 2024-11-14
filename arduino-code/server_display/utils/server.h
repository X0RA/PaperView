#ifndef UTILS_SERVER_H
#define UTILS_SERVER_H

#include <WebServer.h>
#include <Arduino.h>
#include "../config.h"

class DisplayWebServer {
private:
    WebServer server;
    bool *refreshRequested;

public:
    DisplayWebServer(bool *refreshFlag) : server(80), refreshRequested(refreshFlag) {
        LOG_I("Initializing DisplayWebServer on port 80");
        server.on("/refresh", HTTP_POST, [this]() {
            LOG_D("Received POST request to /refresh");
            *refreshRequested = true;
            server.send(200, "text/plain", "Refresh requested");
            LOG_D("Refresh request processed successfully");
        });

        server.on("/refresh", HTTP_GET, [this]() {
            LOG_D("Received GET request to /refresh");
            *refreshRequested = true;
            server.send(200, "text/plain", "Refresh requested");
            LOG_D("Refresh request processed successfully");
        });
    }

    void begin() {
        LOG_I("Starting web server");
        server.begin();
    }

    void handle() {
        server.handleClient();
    }
};

#endif // UTILS_SERVER_H