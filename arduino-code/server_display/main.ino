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

// for the base url
#include "utils/wifi_config.h"

ElementManager elementManager;

TouchDrvGT911 touch;
uint8_t *framebuffer;

bool fetchAndDisplayImage(uint8_t *framebuffer, int16_t pos_x = EPD_WIDTH / 2, int16_t pos_y = EPD_HEIGHT / 2, bool invert = false)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return false;
    }

    HTTPClient http;
    Serial.println("Fetching image data...");
    String url = String(BASE_URL) + String("/image?width=100&height=100");
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    // Get width and height from header
    uint32_t img_width, img_height;
    if (http.getStream().readBytes((char *)&img_width, 4) != 4 ||
        http.getStream().readBytes((char *)&img_height, 4) != 4)
    {
        Serial.println("Failed to read image dimensions");
        http.end();
        return false;
    }
    Serial.printf("Image dimensions: %dx%d\n", img_width, img_height);

    // Validate image will fit on display
    if (pos_x + img_width > EPD_WIDTH || pos_y + img_height > EPD_HEIGHT ||
        pos_x < 0 || pos_y < 0)
    {
        Serial.println("Image position out of bounds");
        http.end();
        return false;
    }

    // Calculate sizes
    size_t bytes_per_row = img_width / 2; // 4-bit grayscale = 2 pixels per byte
    size_t image_data_size = bytes_per_row * img_height;
    size_t display_bytes_per_row = EPD_WIDTH / 2;

    // Allocate temporary buffer for the image data
    uint8_t *img_buffer = (uint8_t *)malloc(image_data_size);
    if (!img_buffer)
    {
        Serial.println("Failed to allocate image buffer");
        http.end();
        return false;
    }

    // Read image data
    if (http.getStream().readBytes((char *)img_buffer, image_data_size) != image_data_size)
    {
        Serial.println("Failed to read complete image data");
        free(img_buffer);
        http.end();
        return false;
    }

    // Calculate byte offset for x position (2 pixels per byte)
    uint16_t x_byte_offset = pos_x / 2;
    uint8_t x_pixel_offset = pos_x % 2;

    // Copy image data into framebuffer at correct position
    for (uint32_t y = 0; y < img_height; y++)
    {
        for (uint32_t x = 0; x < img_width; x++)
        {
            // Calculate source byte and nibble
            uint32_t src_byte_idx = (y * bytes_per_row) + (x / 2);
            bool is_high_nibble = (x % 2 == 0);
            uint8_t pixel;

            // Extract the pixel value (4 bits)
            if (is_high_nibble)
            {
                pixel = (img_buffer[src_byte_idx] >> 4) & 0x0F;
            }
            else
            {
                pixel = img_buffer[src_byte_idx] & 0x0F;
            }

            // Handle inversion and transparency
            if (invert)
            {
                if (pixel == 0x0F)
                    pixel = 0x00;
                else if (pixel == 0x00)
                    pixel = 0x0F;

                // Skip black pixels for transparency
                if (pixel == 0x00)
                    continue;
            }
            else
            {
                // skip white pixels for transparency
                if (pixel == 0x0F)
                    continue;
            }

            // Calculate destination position
            uint32_t dst_x = pos_x + x;
            uint32_t dst_y = pos_y + y;
            uint32_t dst_byte_idx = (dst_y * display_bytes_per_row) + (dst_x / 2);
            bool dst_is_high_nibble = (dst_x % 2 == 0);

            // Write the pixel to the framebuffer
            if (dst_is_high_nibble) {
                framebuffer[dst_byte_idx] = (framebuffer[dst_byte_idx] & 0x0F) | (pixel << 4);
            } else {
                framebuffer[dst_byte_idx] = (framebuffer[dst_byte_idx] & 0xF0) | pixel;
            }
        }
    }

    free(img_buffer);
    http.end();
    return true;
}

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
    set_black_display_mode();
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

    fetchAndDisplayImage(framebuffer);

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