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

    esp_err_t wifi_wakeup = esp_sleep_enable_wifi_wakeup();
    if (wifi_wakeup != ESP_OK)
    {
        Serial.printf("Failed to enable wifi wakeup: %d\n", wifi_wakeup);
    }

    // Configure WiFi for power saving
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

void setupDisplay()
{
    epd_poweron();
    epd_clear();
    set_background(framebuffer);
    epd_poweroff();
}
// // setup sleep
// void setupSleep()
// {
//     // Configure wake-up sources
//     esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 seconds in microseconds

//     // Use negative edge trigger (1->0) for touch wakeup
//     esp_err_t touch_wakeup = esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
//     if (touch_wakeup != ESP_OK)
//     {
//         Serial.printf("Failed to enable touch wakeup: %d\n", touch_wakeup);
//     }

//     // Add debug print before configuring WiFi
//     Serial.printf("TOUCH_INT state: %d\n", digitalRead(TOUCH_INT));
// }

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }

    epd_init();
    setupFramebuffer();
    setupTouch();
    setupWiFi();
    setupDisplay();
    setupSD();
    // setupSleep();
}

// void loop()
// {
//     static unsigned long lastUpdate = 0;
//     static unsigned long lastTouchUpdate = 0;
//     static bool firstRun = true;
//     static bool touchActive = false;
//     static int16_t x, y;

//     const unsigned long TOUCH_DEBOUNCE_TIME = 2000;
//     const unsigned long FIRST_RUN_DELAY = 1000;

//     unsigned long currentTime = millis();

//     // Handle first run
//     if (firstRun && (currentTime >= FIRST_RUN_DELAY))
//     {
//         if (update_display(framebuffer, elementManager, false))
//         {
//             firstRun = false;
//             lastUpdate = currentTime;
//         }
//         return;
//     }

//     bool shouldUpdate = false;

//     // Check wake-up reason
//     esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
//     switch (wakeup_reason)
//     {
//     case ESP_SLEEP_WAKEUP_TIMER:
//         shouldUpdate = true;
//         break;
//     case ESP_SLEEP_WAKEUP_EXT0: // Changed from ESP_SLEEP_WAKEUP_TOUCHPAD
//         if (touch.getPoint(&x, &y))
//         {
//             Serial.printf("Touch detected at X:%d Y:%d\n", x, y);
//             shouldUpdate = elementManager.handleTouch(x, y, framebuffer);
//         }
//         break;
//     default:
//         break;
//     }

//     if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED)
//     {
//         Serial.print("Woke up from sleep. Reason: ");
//         switch (wakeup_reason)
//         {
//         case ESP_SLEEP_WAKEUP_TIMER:
//             Serial.println("Timer");
//             break;
//         case ESP_SLEEP_WAKEUP_EXT0: // Changed from ESP_SLEEP_WAKEUP_TOUCHPAD
//             Serial.println("Touchpad");
//             break;
//         default:
//             Serial.println("Other");
//             break;
//         }
//     }

//     // Handle web server requests
//     webServer.handle();

//     // Update display if needed
//     if (shouldUpdate || refreshRequested)
//     {
//         if (update_display(framebuffer, elementManager, false))
//         {
//             lastUpdate = currentTime;
//         }
//         refreshRequested = false;
//     } else {
//         // Only enter sleep if pin is in expected state
//         int touchState = digitalRead(TOUCH_INT);
//         Serial.printf("TOUCH_INT state before sleep: %d\n", touchState);

//         if (touchState == HIGH) {  // Only sleep if pin is HIGH (no touch active)
//             Serial.println("Entering light sleep mode...");
//             esp_light_sleep_start();
//         } else {
//             Serial.println("Skipping sleep - touch pin is LOW");
//             delay(100);  // Small delay before checking again
//         }
//     }
// }

void loop()
{
    static unsigned long lastUpdate = 0;
    static unsigned long lastTouchUpdate = 0;
    static bool firstRun = true;
    static bool touchActive = false;
    static int16_t x, y;

    const unsigned long TOUCH_DEBOUNCE_TIME = 2000;
    const unsigned long AUTO_UPDATE_INTERVAL = 60000;
    const unsigned long FIRST_RUN_DELAY = 1000;

    unsigned long currentTime = millis();

    // Handle first run
    if (firstRun && (currentTime >= FIRST_RUN_DELAY))
    {
        if (update_display(framebuffer, elementManager, false))
        {
            firstRun = false;
            lastUpdate = currentTime;
        }
        return;
    }

    // Handle automatic update interval
    if (!firstRun && (currentTime - lastUpdate >= AUTO_UPDATE_INTERVAL))
    {
        if (update_display(framebuffer, elementManager, false))
        {
            lastUpdate = currentTime;
        }
    }

    // Handle touch input
    if (touch.getPoint(&x, &y))
    {
        if (!touchActive && (currentTime - lastTouchUpdate >= TOUCH_DEBOUNCE_TIME))
        {
            Serial.printf("Touch detected at X:%d Y:%d\n", x, y);
            bool refresh = elementManager.handleTouch(x, y, framebuffer);
            if (refresh)
            {
                update_display(framebuffer, elementManager, false);
            }
            lastTouchUpdate = currentTime;
        }
        touchActive = true;
    }
    else
    {
        touchActive = false;
    }

    // Handle web server requests
    webServer.handle();

    // Check for refresh requests
    if (refreshRequested)
    {
        if (update_display(framebuffer, elementManager, false))
        {
            lastUpdate = currentTime;
        }
        refreshRequested = false;
    }

    delay(10);
}