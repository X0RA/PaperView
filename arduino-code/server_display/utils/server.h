#ifndef UTILS_SERVER_H
#define UTILS_SERVER_H

#include <WebServer.h>
#include <Arduino.h>

class DisplayWebServer {
private:
    WebServer server;
    bool *refreshRequested;

public:
    DisplayWebServer(bool *refreshFlag) : server(80), refreshRequested(refreshFlag) {
        server.on("/refresh", HTTP_POST, [this]() {
            *refreshRequested = true;
            server.send(200, "text/plain", "Refresh requested");
        });

        server.on("/refresh", HTTP_GET, [this]() {
            *refreshRequested = true;
            server.send(200, "text/plain", "Refresh requested");
        });
    }

    void begin() {
        server.begin();
    }

    void handle() {
        server.handleClient();
    }
};

#endif // UTILS_SERVER_H