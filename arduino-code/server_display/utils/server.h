#ifndef UTILS_SERVER_H
#define UTILS_SERVER_H

#include <WebServer.h>
#include <Arduino.h>
#include <atomic>
#include "../config.h"
#include "./types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class DisplayWebServer {
private:
    WebServer server;
    std::atomic<RefreshType> *refreshRequested;
    TaskHandle_t serverTaskHandle;

    static void serverTaskWrapper(void *parameter) {
        DisplayWebServer *server = (DisplayWebServer *)parameter;
        server->serverTask();
    }

    void serverTask() {
        LOG_I("Starting web server");
        server.begin();

        while (true) {
            server.handleClient();
            vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent task from hogging CPU
        }
    }

public:
    DisplayWebServer(std::atomic<RefreshType> *refreshFlag) : server(80),
                                                              refreshRequested(refreshFlag),
                                                              serverTaskHandle(NULL) {
        LOG_I("Initializing DisplayWebServer on port 80");

        server.on("/full_refresh", HTTP_GET, [this]() {
            LOG_D("Received GET request to /full_refresh");
            refreshRequested->store(FULL_REFRESH);
            server.send(200, "text/plain", "Full refresh requested");
            LOG_D("Full refresh request processed successfully");
        });

        server.on("/soft_refresh", HTTP_GET, [this]() {
            LOG_D("Received GET request to /soft_refresh");
            refreshRequested->store(SOFT_REFRESH);
            server.send(200, "text/plain", "Soft refresh requested");
            LOG_D("Soft refresh request processed successfully");
        });

        // Create the server handling thread
        xTaskCreate(
            serverTaskWrapper, // Task function
            "ServerTask",      // Task name
            4096,              // Stack size (bytes)
            this,              // Task parameters
            1,                 // Priority
            &serverTaskHandle  // Task handle
        );
    }

    ~DisplayWebServer() {
        if (serverTaskHandle != NULL) {
            vTaskDelete(serverTaskHandle);
        }
    }
};

#endif // UTILS_SERVER_H