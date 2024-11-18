// Base
#include <Arduino.h>
#include "epd_driver.h"

// Wi-Fi
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <HTTPClient.h>

// SD
#include "utils/sdcard.h"

// touch
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "firasans.h"
#include <Wire.h>
#include <TouchDrvGT911.hpp>
// #include "utilities.h"

// utils
#include "utils/api_functions.h"
#include "utils/wifi_config.h"
#include "utils/connect_wifi.h"
#include "utils/web_server.h"

// managers
#include "managers/element_manager.h"
#include "managers/display_manager.h"
#include "managers/lifecycle_manager.h"

ElementManager elementManager;
TouchDrvGT911 touch;
uint8_t *framebuffer;

// Web server vars
bool refreshRequested = false;
DisplayWebServer webServer(&refreshRequested);

void setupFramebuffer()
{
    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer)
    {
        Serial.println("alloc memory failed !!!");
        while (1)
            ;
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

void setupTouch()
{
    //* Sleep wakeup must wait one second, otherwise the touch device cannot be addressed
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED)
    {
        delay(1000);
    }

    Wire.begin(BOARD_SDA, BOARD_SCL);

    // Initialize touch pin properly
    pinMode(TOUCH_INT, INPUT_PULLUP); // Start with INPUT_PULLUP
    delay(100);                       // Give time for pin to stabilize

    // Wake up touch controller with brief pulse
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_INT, HIGH);
    delay(100);
    pinMode(TOUCH_INT, INPUT_PULLUP); // Return to INPUT_PULLUP
    delay(100);                       // Allow time to stabilize

    /*
     * The touch reset pin uses hardware pull-up,
     * and the function of setting the I2C device address cannot be used.
     * Use scanning to obtain the touch device address.*/
    uint8_t touchAddress = 0;
    Wire.beginTransmission(0x14);
    if (Wire.endTransmission() == 0)
    {
        touchAddress = 0x14;
    }
    Wire.beginTransmission(0x5D);
    if (Wire.endTransmission() == 0)
    {
        touchAddress = 0x5D;
    }
    if (touchAddress == 0)
    {
        while (1)
        {
            Serial.println("Failed to find GT911 - check your wiring!");
            delay(1000);
        }
    }
    touch.setPins(-1, TOUCH_INT);
    if (!touch.begin(Wire, touchAddress, BOARD_SDA, BOARD_SCL))
    {
        while (1)
        {
            Serial.println("Failed to find GT911 - check your wiring!");
            delay(1000);
        }
    }
    touch.setMaxCoordinates(EPD_WIDTH, EPD_HEIGHT);
    touch.setSwapXY(true);
    touch.setMirrorXY(false, true);

    Serial.println("Started Touchscreen poll...");
}

void setupWiFi()
{
    String wifiRes = connectToWiFi();
    Serial.println(wifiRes);
    webServer.begin();
}

void setupDisplay()
{
    epd_poweron();
    epd_clear();
    set_black_display_mode();
    set_background(framebuffer);
    epd_poweroff();
}

// setup sleep
void setupSleep()
{
    // // Configure wake-up sources
    // esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 seconds in microseconds

    // // Use negative edge trigger (1->0) for touch wakeup
    // esp_err_t touch_wakeup = esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
    // if (touch_wakeup != ESP_OK)
    // {
    //     Serial.printf("Failed to enable touch wakeup: %d\n", touch_wakeup);
    // }

    // // Add debug print before configuring WiFi
    // Serial.printf("TOUCH_INT state: %d\n", digitalRead(TOUCH_INT));

    esp_err_t wifi_wakeup = esp_sleep_enable_wifi_wakeup();
    if (wifi_wakeup != ESP_OK)
    {
        Serial.printf("Failed to enable wifi wakeup: %d\n", wifi_wakeup);
    }

    // Configure WiFi for power saving but maintain connection
    WiFi.setSleep(true);
    // esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }

    epd_init();
    setupFramebuffer();
    // setupTouch();
    setupWiFi();
    setupDisplay();
    // setupSD();
    setupSleep();
}

String getWakeupReason(esp_sleep_wakeup_cause_t wakeup_reason)
{
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            return "Undefined";
        case ESP_SLEEP_WAKEUP_ALL:
            return "All Sources"; 
        case ESP_SLEEP_WAKEUP_EXT0:
            return "External Signal 0";
        case ESP_SLEEP_WAKEUP_EXT1:
            return "External Signal 1";
        case ESP_SLEEP_WAKEUP_TIMER:
            return "Timer";
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return "Touchpad";
        case ESP_SLEEP_WAKEUP_ULP:
            return "ULP Coprocessor";
        case ESP_SLEEP_WAKEUP_GPIO:
            return "GPIO";
        case ESP_SLEEP_WAKEUP_UART:
            return "UART";
        case ESP_SLEEP_WAKEUP_WIFI:
            return "WiFi";
        case ESP_SLEEP_WAKEUP_COCPU:
            return "Coprocessor";
        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
            return "Coprocessor Trap";
        case ESP_SLEEP_WAKEUP_BT:
            return "Bluetooth";
        default:
            return "Unknown";
    }
}

void loop()
{
    // static unsigned long lastUpdate = 0;
    // static bool firstRun = true;
    // unsigned long currentTime = millis();

    // if (firstRun)
    // {
    //     if (update_display(framebuffer, elementManager, false))
    //     {
    //         firstRun = false;
    //         lastUpdate = currentTime;
    //     }
    //     return;
    // }

    // Check wake-up reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    String wakeup_reason_name = getWakeupReason(wakeup_reason);
    Serial.printf("Wakeup reason name: %s\n", wakeup_reason_name.c_str());
    webServer.handle();
    
    // Use delay instead of immediate sleep to allow web server to work
    delay(100);  // Small delay to process requests

    
    // Use light sleep with WiFi maintained
    esp_sleep_enable_wifi_wakeup();
    esp_light_sleep_start();
}