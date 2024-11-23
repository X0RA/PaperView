#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include "../config.h"
#include "../managers/element_manager.h"
#include "../utils/eink.h"
#include "../utils/http.h"
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

class ApplicationController {
private:
#pragma region Properties
    uint8_t *const framebuffer;
    ElementManager elementManager;
    DisplayWebServer webServer;

    // Touch
    TouchDrvGT911 &touch;
    TaskHandle_t touchTaskHandle;
    volatile bool touch_active;
    unsigned long last_touch_time;
    unsigned long last_update_time;

    // Refresh
    std::atomic<RefreshType> refresh_type{DISPLAY_REFRESH_COMPLETE};

    static void touchTaskWrapper(void *parameter) {
        ApplicationController *controller = (ApplicationController *)parameter;
        controller->touchTask();
    }

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

#pragma region Private Methods

    void checkAutoUpdate() {
        unsigned long current_time = millis();
        unsigned long time_since_update = current_time - last_update_time;

        // Check longest interval first
        if (time_since_update >= DISPLAY_REFRESH_COMPLETE_INTERVAL) {
            refresh_type.store(DISPLAY_REFRESH_COMPLETE);
        } else if (time_since_update >= DISPLAY_REFRESH_PARTIAL_INTERVAL) {
            refresh_type.store(DISPLAY_REFRESH_PARTIAL);
        } else if (time_since_update >= DISPLAY_REFRESH_FAST_INTERVAL) {
            refresh_type.store(DISPLAY_REFRESH_FAST);
        }

        // Only update the time if we actually set a refresh type
        if (time_since_update >= DISPLAY_REFRESH_FAST_INTERVAL) {
            last_update_time = current_time;
        }
    }

    /**
     * @brief Update the display from an API response
     * @param response The API response to update the display from
     * @return Whether the elements were updated successfully
     * true = elements updated, display needs to be redrawn
     * false = no elements updated, display does not need to be redrawn (keeps last image)
     */
    bool updateDisplayFromResponse(ApiResponse_t &response) {
        if (!response.success) {
            LOG_E("Failed to get API response");
            return false;
        }

        DynamicJsonDocument doc = parseJsonResponse(response.response);
        if (doc.isNull()) {
            LOG_E("Failed to parse JSON data");
            return false;
        }

        JsonArray elements_data = doc["elements"];
        if (elements_data.isNull()) {
            LOG_E("No elements found in JSON");
            return false;
        }

        elementManager.processElements(elements_data);
        return true;
    }

#pragma endregion

public:
#pragma region Instantiation Methods
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

#pragma region Public Methods

    /**
     * @brief Main loop for the display lifecycle
     */
    void loop() {
        // check if we need to update the display based on timer
        checkAutoUpdate();

        // Let element manager handle its own rendering
        elementManager.loop();

        // update display if needed
        RefreshType current_refresh = refresh_type.load();
        if (current_refresh != NO_REFRESH) {
            refresh_display(current_refresh, framebuffer);
            ApiResponse_t response = getPage();
            if (updateDisplayFromResponse(response)) {
                draw_framebuffer(framebuffer);
                refresh_type.store(NO_REFRESH);
            }
        }

        delay(20); // 20ms delay
    }

#pragma endregion
};

#endif // APPLICATION_CONTROLLER_H