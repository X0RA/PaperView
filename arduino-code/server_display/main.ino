// Base
#include "epd_driver.h"
#include <Arduino.h>
#include <utilities.h>
#include "config.h"

// Wi-Fi
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_wifi.h>

// SD
#include "utils/storage.h"

// touch
#include "firasans.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <TouchDrvGT911.hpp>
#include <Wire.h>
#include <esp_task_wdt.h>

// utils
#include "utils/wifi.h"
#include "utils/layout.h"
#include "utils/eink.h"
#include "utils/http.h"
#include "utils/types.h"
// controllers
#include "controllers/application_controller.h"

// Physical inputs
TouchDrvGT911 touch;

// screen buffer

// application controller
ApplicationController *app;

#pragma region Setup Functions

// New framebuffer initialization
std::unique_ptr<uint8_t[]> framebuffer;
void setupFramebuffer() {
    framebuffer = std::unique_ptr<uint8_t[]>(new uint8_t[EPD_WIDTH * EPD_HEIGHT / 2]);
    if (!framebuffer) {
        LOG_E("Failed to allocate framebuffer memory");
        return;
    }
    LOG_D("Allocated framebuffer of size %d bytes", EPD_WIDTH * EPD_HEIGHT / 2);
    memset(framebuffer.get(), 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    LOG_I("Framebuffer initialized successfully");
}

// // old method for framebuffer
// uint8_t *framebuffer = NULL;
// void setupFramebuffer() {
//     framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
//     if (!framebuffer) {
//         LOG_E("Failed to allocate framebuffer memory");
//         while (1);
//     }
//     memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
// }

void setupTouch() {
    //* Sleep wakeup must wait one second, otherwise the touch device cannot be
    // addressed
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) {
        LOG_D("Sleep wakeup detected, waiting 1s");
        delay(1000);
    }

    Wire.begin(BOARD_SDA, BOARD_SCL);
    LOG_D("Started I2C on SDA:%d SCL:%d", BOARD_SDA, BOARD_SCL);

    // Initialize touch pin properly
    pinMode(TOUCH_INT, INPUT_PULLUP); // Start with INPUT_PULLUP
    delay(100);                       // Give time for pin to stabilize
    LOG_D("Touch INT pin initialized as INPUT_PULLUP");

    // Wake up touch controller with brief pulse
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_INT, HIGH);
    delay(100);
    pinMode(TOUCH_INT, INPUT_PULLUP); // Return to INPUT_PULLUP
    delay(100);                       // Allow time to stabilize
    LOG_D("Touch controller wake pulse sent");

    /*
     * The touch reset pin uses hardware pull-up,
     * and the function of setting the I2C device address cannot be used.
     * Use scanning to obtain the touch device address.*/
    uint8_t touchAddress = 0;
    Wire.beginTransmission(0x14);
    if (Wire.endTransmission() == 0) {
        touchAddress = 0x14;
    }
    Wire.beginTransmission(0x5D);
    if (Wire.endTransmission() == 0) {
        touchAddress = 0x5D;
    }
    if (touchAddress == 0) {
        while (1) {
            LOG_E("Failed to find GT911 - check your wiring!");
            delay(1000);
        }
    }
    LOG_I("Found GT911 at address 0x%02X", touchAddress);

    touch.setPins(-1, TOUCH_INT);
    if (!touch.begin(Wire, touchAddress, BOARD_SDA, BOARD_SCL)) {
        while (1) {
            LOG_E("Failed to initialize GT911 - check your wiring!");
            delay(1000);
        }
    }
    touch.setMaxCoordinates(EPD_WIDTH, EPD_HEIGHT);
    touch.setSwapXY(true);
    touch.setMirrorXY(false, true);

    LOG_I("Touchscreen initialized successfully");
}

void setupWiFi() {
    connectToWiFi();

    esp_err_t wifi_wakeup = esp_sleep_enable_wifi_wakeup();
    if (wifi_wakeup != ESP_OK) {
        LOG_E("Failed to enable wifi wakeup: %d\n", wifi_wakeup);
    }

    esp_err_t ps_result = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    if (ps_result != ESP_OK) {
        LOG_E("Failed to set WiFi power save mode: %d", ps_result);
    }
}
#pragma endregion
void setup() {
    Serial.begin(115200);
    // This code would wait for a serial connection before continuing,
    // comment out to allow the device to run without being connected to a computer
    // while (!Serial) {
    //     delay(10);
    // }
    epd_init();
    setupFramebuffer();
    setupTouch();
    setupWiFi();
    setupSD();
    app = new ApplicationController(framebuffer.get(), touch);
    LOG_I("Initialized successfully");
}

void loop() {
    app->loop();
}