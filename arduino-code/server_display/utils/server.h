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

        server.on("/complete_refresh", HTTP_GET, [this]() {
            LOG_D("Received GET request to /complete_refresh");
            refreshRequested->store(DISPLAY_REFRESH_COMPLETE);
            server.send(200, "text/plain", "Complete refresh requested");
            LOG_D("Complete refresh request processed successfully");
        });

        server.on("/partial_refresh", HTTP_GET, [this]() {
            LOG_D("Received GET request to /partial_refresh");
            refreshRequested->store(DISPLAY_REFRESH_PARTIAL);
            server.send(200, "text/plain", "Partial refresh requested");
            LOG_D("Partial refresh request processed successfully");
        });

        server.on("/fast_refresh", HTTP_GET, [this]() {
            LOG_D("Received GET request to /fast_refresh");
            refreshRequested->store(DISPLAY_REFRESH_FAST);
            server.send(200, "text/plain", "Fast refresh requested");
            LOG_D("Fast refresh request processed successfully");
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