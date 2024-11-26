#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include "../config.h"
#include "../managers/element_manager.h"
#include "../utils/eink.h"
#include "../utils/http.h"
#include "../utils/wifi.h"
#include "../utils/server.h"
#include "../utils/storage.h"
#include "../utils/types.h"
#include "epd_driver.h"
#include <Arduino.h>
#include <atomic>

// touch
#include "firasans.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <TouchDrvGT911.hpp>
#include <Wire.h>
#include <esp_task_wdt.h>

// for reference, from types.h:
// enum RefreshType {
//     NO_REFRESH,               // No refresh (default)
//     REFETCH_ELEMENTS,         // Refresh just the elements from the API to the element manager
//     ELEMENT_REFRESH_FAST,     // Fast refresh - Flash background color on area before drawing
//     ELEMENT_REFRESH_PARTIAL,  // Partial refresh - 2 cycle black to white flash on area before drawing
//     ELEMENT_REFRESH_COMPLETE, // Complete refresh - 4 cycles black to white flash on area before drawing
//     DISPLAY_REFRESH_FAST,     // Fast refresh - Flash background color on area before drawing
//     DISPLAY_REFRESH_PARTIAL,  // Partial refresh - 2 cycle black to white flash on entire display
//     DISPLAY_REFRESH_COMPLETE  // Complete refresh - 4 cycles black to white flash on entire display
// };

class ApplicationController {
private:
#pragma region Properties and touch task
    uint8_t *const framebuffer;
    ElementManager elementManager;
    DisplayWebServer webServer;

    // Touch variables
    TouchDrvGT911 &touch;
    TaskHandle_t touchTaskHandle;
    volatile bool touch_active;
    unsigned long last_touch_time;
    unsigned long last_update_time;

    // wifi variables
    unsigned long last_wifi_check_time;

    // Atomic refresh type, used to trigger a refresh of the display or elements.
    // Atomic used to ensure thread safety when updating the refresh type from multiple threads.
    std::atomic<RefreshType> refresh_type{DISPLAY_REFRESH_COMPLETE};

    static void touchTaskWrapper(void *parameter) {
        ApplicationController *controller = (ApplicationController *)parameter;
        controller->touchTask();
    }

    /**
     * @brief Async task that handles touch events, sending the x,y coordinates to the element manager to determine if the touch was on an element.
     * It will trigger a refetch of the elements from the API if an element was touched.
     */
    void touchTask() {
        int16_t x, y;
        while (true) {
            // Reset watchdog timer
            esp_task_wdt_reset();

            unsigned long current_time = millis();

            if (touch.getPoint(&x, &y)) {
                if (!touch_active &&
                    (current_time - last_touch_time >= TOUCH_DEBOUNCE_TIME)) {
                    LOG_D("Touch at X:%d Y:%d", x, y);
                    if (elementManager.handleTouch(x, y)) {
                        refresh_type.store(REFETCH_ELEMENTS);
                    }
                    last_touch_time = current_time;
                }
                touch_active = true;
            } else {
                touch_active = false;
            }

            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }

#pragma endregion

#pragma region Private Methods (checkScreenRefresh, fetchElementsFromAPI)

    /**
     * @brief Checks if the display needs to be refreshed based on the last update time.
     * It will set the refresh type to the appropriate type based on the time since the last whole screen refresh.
     * #define ELEMENT_REFRESH_INTERVAL 30000            // ms (30 seconds) when the page is updated (no flashing)
     * #define DISPLAY_REFRESH_FAST_INTERVAL 150000      // ms (2.5 minutes) when the page is updated (no flashing)
     * #define DISPLAY_REFRESH_PARTIAL_INTERVAL 300000   // ms (5 minutes) when the page is updated (with flashing)
     * #define DISPLAY_REFRESH_COMPLETE_INTERVAL 1800000 // ms (30 minutes) when the page is updated (with flashing)

     */
    void checkScreenRefresh() {
        unsigned long current_time = millis();
        unsigned long time_since_update = current_time - last_update_time;

        // Store the most aggressive refresh type needed
        RefreshType needed_refresh = NO_REFRESH;

        // Check all intervals and use the most aggressive refresh type
        if (time_since_update >= DISPLAY_REFRESH_COMPLETE_INTERVAL) {
            needed_refresh = DISPLAY_REFRESH_COMPLETE;
        } else if (time_since_update >= DISPLAY_REFRESH_PARTIAL_INTERVAL) {
            needed_refresh = DISPLAY_REFRESH_PARTIAL;
        } else if (time_since_update >= DISPLAY_REFRESH_FAST_INTERVAL) {
            needed_refresh = DISPLAY_REFRESH_FAST;
        } else if (time_since_update >= ELEMENT_REFRESH_INTERVAL) {
            needed_refresh = REFETCH_ELEMENTS;
        }

        // Only update if we need a refresh and it's more aggressive than current
        if (needed_refresh != NO_REFRESH &&
            needed_refresh > refresh_type.load()) {
            refresh_type.store(needed_refresh);
            last_update_time = current_time;
        }
    }

    /**
     * @brief Checks if the WiFi connection is active, and attempts to connect if not.
     */
    void checkWifiConnection() {
        unsigned long current_time = millis();

        // Only check wifi connection after interval has passed
        if (current_time - last_wifi_check_time < WIFI_CHECK_INTERVAL) {
            return;
        }

        last_wifi_check_time = current_time;
        bool connected = isWiFiConnected();

        if (connected) {
            return;
        }

        while (!connected) {
            connectToWiFi();
            connected = isWiFiConnected();
            if (connected) {
                break;
            }
            delay(500);
        }
    }

    /**
     * @brief Fetches the elements from the API and sends them to the element manager for processing.
     */
    void fetchElementsFromAPI() {
        ApiResponse_t response = getPage();

        if (!response.success) {
            LOG_E("Failed to get API response");
            return;
        }

        DynamicJsonDocument doc = parseJsonResponse(response.response);
        if (doc.isNull()) {
            LOG_E("Failed to parse JSON data");
            return;
        }

        JsonArray elements_data = doc["elements"];
        if (elements_data.isNull()) {
            LOG_E("No elements found in JSON");
            return;
        }

        elementManager.processElements(elements_data);
    }

#pragma endregion

public:
#pragma region Instantiation Methods including touch thread
    ApplicationController(uint8_t *framebuffer, TouchDrvGT911 &touch) : framebuffer(framebuffer),
                                                                        touch_active(false),
                                                                        last_touch_time(0),
                                                                        last_update_time(0),
                                                                        touch(touch),
                                                                        elementManager(framebuffer),
                                                                        webServer(&refresh_type),
                                                                        touchTaskHandle(NULL) {
        // Create the touch handling thread
        xTaskCreate(
            touchTaskWrapper, // Task function
            "TouchTask",      // Task name
            8192,             // Stack size (bytes) - increased from 4096
            this,             // Task parameters
            1,                // Priority
            &touchTaskHandle  // Task handle
        );

        // Enable watchdog for touch task
        esp_task_wdt_add(touchTaskHandle);
    };

    ~ApplicationController() {
        if (touchTaskHandle != NULL) {
            vTaskDelete(touchTaskHandle);
        }
    }

#pragma endregion

#pragma region Public Methods (loop)

    /**
     * @brief Main application loop that handles:
     * - Periodic display refresh to prevent pixel sticking (checkScreenRefresh and webServer.handleClient)
     * - Fetching new data from API (checkScreenRefresh and touch events)
     * - Sending data to the element manager for processing (fetchElementsFromAPI)
     */
    void loop() {
        // Check if we need to connect to WiFi
        checkWifiConnection();

        // check if we need to update the display based on timer
        checkScreenRefresh();

        // This is where we actually refresh the display and fetch new data from the API
        RefreshType current_refresh = refresh_type.load();
        if (current_refresh != NO_REFRESH) {
            // if current_refresh is REFETCH_ELEMENTS, the display will not flash / unstick pixels and element_manager will handle the refresh for specific areas within it's loop
            refresh_display(current_refresh, framebuffer);
            fetchElementsFromAPI();
            refresh_type.store(NO_REFRESH);
        }

        // Let element manager handle its own sub element rendering and refresh logic
        elementManager.loop();

        delay(20); // 20ms delay
    }

#pragma endregion
};

#endif // APPLICATION_CONTROLLER_H