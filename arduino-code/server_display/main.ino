#include <Arduino.h>
#include "epd_driver.h"

// Wi-Fi
#include <WiFi.h>
#include "utils/connectWifi.h"
#include <ArduinoJson.h>

// HTTP
#include <HTTPClient.h>

// touch
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "firasans.h"
#include <Wire.h>
#include <TouchDrvGT911.hpp>
#include "utilities.h"

// my helpers
#include "drawing/draw_display.h"
#include "utils/api_functions.h"
#include "drawing/ElementManager.h"

ElementManager elementManager;

TouchDrvGT911 touch;
uint8_t *framebuffer;

void setup()
{
    Serial.begin(115200);
    epd_init();

    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer)
    {
        Serial.println("alloc memory failed !!!");
        while (1)
            ;
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    //* Sleep wakeup must wait one second, otherwise the touch device cannot be addressed
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED)
    {
        delay(1000);
    }

    Wire.begin(BOARD_SDA, BOARD_SCL);

    // Assuming that the previous touch was in sleep state, wake it up
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_INT, HIGH);

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

    // Connect to Wi-Fi
    String wifiRes = connectToWiFi();
    Serial.println(wifiRes);

    // setup background
    epd_poweron();
    // set_black_display_mode();
    epd_clear();
    set_background(framebuffer);
    epd_poweroff();
}

bool update_display(uint8_t *framebuffer, bool doClear)
{
    if (!framebuffer)
    {
        Serial.println("Framebuffer is null!");
        return false;
    }

    epd_poweron();

    // Get data from API
    ApiResponse_t getResponse = makeGetRequest(PAGE_HOME);
    if (!getResponse.success)
    {
        Serial.println("Failed to get API response");
        epd_poweroff();
        return false;
    }

    // Parse JSON response
    DynamicJsonDocument doc = parseJsonResponse(getResponse.response);
    if (doc.isNull())
    {
        Serial.println("Failed to parse JSON data");
        epd_poweroff();
        return false;
    }

    // Check if we should clear the display
    bool shouldClear = false;
    if (doc.containsKey("clear"))
    {
        shouldClear = doc["clear"].as<bool>();
        Serial.printf("Clear flag from server: %s\n", shouldClear ? "true" : "false");
    }

    if (shouldClear || doClear)
    {
        Serial.println("Clearing display");
        epd_clear();
        // Also clear framebuffer
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    }

    set_background(framebuffer);

    // Get the elements array
    JsonArray elements = doc["elements"];
    if (elements.isNull())
    {
        Serial.println("No elements found in JSON");
        epd_poweroff();
        return false;
    }

    elementManager.processElements(elements, framebuffer);

    epd_draw_grayscale_image(epd_full_screen(), framebuffer);

    epd_poweroff();
    return true;
}

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
        if (update_display(framebuffer, false))
        {
            firstRun = false;
            lastUpdate = currentTime;
        }
        return;
    }

    // Handle automatic update interval
    if (!firstRun && (currentTime - lastUpdate >= AUTO_UPDATE_INTERVAL))
    {
        if (update_display(framebuffer, false))
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
                update_display(framebuffer, false);
            }
            lastTouchUpdate = currentTime;
        }
        touchActive = true;
    }
    else
    {
        touchActive = false;
    }

    delay(10);
}