#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include "../config.h"
#include "../managers/element_manager.h"
#include "../utils/eink.h"
#include "../utils/http.h"
#include "../utils/server.h"
#include "../utils/storage.h"
#include "epd_driver.h"
#include <Arduino.h>

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
    TouchDrvGT911 &touch;
    DisplayWebServer webServer;

    bool touch_active;
    unsigned long last_touch_time;
    unsigned long last_update_time;
    bool refresh_display;
#pragma endregion

#pragma region Private Methods
    void handleTouch() {
        int16_t x, y;
        unsigned long current_time = millis();

        if (touch.getPoint(&x, &y)) {
            if (!touch_active &&
                (current_time - last_touch_time >= TOUCH_DEBOUNCE_TIME)) {
                LOG_D("Touch at X:%d Y:%d", x, y);
                if (elements.handleTouch(x, y)) {
                    refresh_display = true;
                }
                last_touch_time = current_time;
            }
            touch_active = true;
        } else {
            touch_active = false;
        }
    }

    void checkAutoUpdate() {
        if (millis() - last_update_time >= AUTO_UPDATE_INTERVAL) {
            refresh_display = true;
        }
    }

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

        // Handle clear flag
        if (doc["clear"] | false) {
            LOG_I("Clearing display");
            set_background(framebuffer);
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
                                                                        webServer(&refresh_display) {};

    ~ApplicationController() {
        // No cleanup needed
    }

#pragma endregion

#pragma region Public Methods
    void loop() {
        handleTouch();
        checkAutoUpdate();
        webServer.handle();

        if (refresh_display) {
            updateDisplay();
        }

        delay(10); // 10ms delay
    }

    void updateDisplay() {
        epd_poweron();

        ApiResponse_t response = getPage();
        if (updateDisplayFromResponse(response)) {
            epd_draw_grayscale_image(epd_full_screen(), framebuffer);
            refresh_display = false;
            last_update_time = millis();
        }

        epd_poweroff();
    }

    void requestRefresh() {
        refresh_display = true;
    }
#pragma endregion
};

#endif // APPLICATION_CONTROLLER_H