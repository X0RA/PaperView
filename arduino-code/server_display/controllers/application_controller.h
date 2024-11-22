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
    ElementManager elements;
    DisplayWebServer webServer;

    // Touch
    TouchDrvGT911 &touch;
    TaskHandle_t touchTaskHandle;
    volatile bool touch_active;
    unsigned long last_touch_time;
    unsigned long last_update_time;

    // Refresh
    std::atomic<RefreshType> refresh_type{NO_REFRESH};

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
                    if (elements.handleTouch(x, y)) {
                        refresh_type.store(SOFT_REFRESH);
                    }
                    last_touch_time = current_time;
                }
                touch_active = true;
            } else {
                touch_active = false;
            }

            // Increased delay to give more time to other tasks
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

#pragma endregion

#pragma region Private Methods

    void checkAutoUpdate() {
        unsigned long current_time = millis();
        unsigned long time_since_update = current_time - last_update_time;

        if (time_since_update >= AUTO_FULL_UPDATE_INTERVAL) {
            refresh_type.store(FULL_REFRESH);
            last_update_time = current_time;
        } else if (time_since_update >= AUTO_SOFT_UPDATE_INTERVAL) {
            refresh_type.store(SOFT_REFRESH);
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

        elements.processElements(elements_data);
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
                                                                        elements(framebuffer),
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

        // Render touched elements
        if (elements.renderTouched()) {
            drawDisplay();
        }

        // update display if needed, this function also turn on the epd display
        RefreshType current_refresh = refresh_type.load();
        if (current_refresh != NO_REFRESH) {
            if (current_refresh == FULL_REFRESH) {
                fullRefresh();
            }
            if (current_refresh == SOFT_REFRESH) {
                softRefresh();
            }
        }

        delay(20); // 20ms delay
    }

    /**
     * @brief Perform a full refresh of the display by clearing the screen four times
     */
    void fullRefresh() {
        LOG_D("Full refreshing display");
        full_refresh();
        ApiResponse_t response = getPage();
        if (updateDisplayFromResponse(response)) {
            drawDisplay();
            refresh_type.store(NO_REFRESH);
            last_update_time = millis();
        }
    }

    /**
     * @brief Perform a soft refresh of the display by clearing the framebuffer and drawing/rendering the page over existing
     */
    void softRefresh() {
        LOG_D("Soft refreshing display");
        set_background(framebuffer);
        ApiResponse_t response = getPage();
        if (updateDisplayFromResponse(response)) {
            drawDisplay();
            refresh_type.store(NO_REFRESH);
            last_update_time = millis();
        }
    }

    /**
     * @brief Draw the display by drawing the framebuffer to the epd
     */
    void drawDisplay() {
        epd_poweron();
        epd_draw_grayscale_image(epd_full_screen(), framebuffer);
        epd_poweroff();
    }

#pragma endregion
};

#endif // APPLICATION_CONTROLLER_H