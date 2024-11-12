#ifndef IMAGE_ELEMENT_H
#define IMAGE_ELEMENT_H

#include "DrawElement.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "../utils/api_functions.h"


class ImageElement : public DrawElement
{
private:
    char *callback;
    char *name;
    char *path;
    bool touched;
    bool inverted;
    bool filled;
    int16_t width;
    int16_t height;
    // The following are inherited from DrawElement and are here for reference.
    // uint16_t id;
    // char *text; // Unused now
    // int16_t x;
    // int16_t y;
    // Anchor anchor;
    // FontProperties props;
    // Rect_t bounds;
    // bool active;
    // bool changed;
    // bool updated;

public:
    ImageElement() : DrawElement() {
        callback = nullptr;
        name = nullptr;
        path = nullptr;
    }

    ~ImageElement() {
        if (callback) {
            free(callback);
        }
        if (name) {
            free(name);
        }
        if (path) {
            free(path);
        }
    }

    void draw(uint8_t *framebuffer) override
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi not connected!");
            return;
        }

        HTTPClient http;
        Serial.println("Fetching image data...");

        String url = String(BASE_URL) + "/";
        
        if (strncmp(path, "icon", 4) == 0) {
            url += String(path) + "/" + String(name) + 
                   "?width=" + String(width) + 
                   "&height=" + String(height) +
                   "&filled=" + String(filled);
        }
        else if (strncmp(path, "album-art", 9) == 0) {
            url += String(path) + "/" +
                   "?width=" + String(width) +
                   "&height=" + String(height); 
        }
        else {
            Serial.println("Invalid path type");
            return;
        }

        Serial.println("URL: " + url);

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode != HTTP_CODE_OK)
        {
            Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
            http.end();
            return;
        }

        // Get width and height from header
        uint32_t img_width, img_height;
        if (http.getStream().readBytes((char *)&img_width, 4) != 4 ||
            http.getStream().readBytes((char *)&img_height, 4) != 4)
        {
            Serial.println("Failed to read image dimensions");
            http.end();
            return;
        }
        Serial.printf("Image dimensions: %dx%d\n", img_width, img_height);

        // Validate image will fit on display
        if (x + img_width > EPD_WIDTH || y + img_height > EPD_HEIGHT ||
            x < 0 || y < 0)
        {
            Serial.println("Image position out of bounds");
            http.end();
            return;
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
            return;
        }

        // Read image data
        if (http.getStream().readBytes((char *)img_buffer, image_data_size) != image_data_size)
        {
            Serial.println("Failed to read complete image data");
            free(img_buffer);
            http.end();
            return;
        }

        // Calculate byte offset for x position (2 pixels per byte)
        uint16_t x_byte_offset = x / 2;
        uint8_t x_pixel_offset = x % 2;

        // Copy image data into framebuffer at correct position
        for (uint32_t pos_y = 0; pos_y < img_height; pos_y++)
        {
            for (uint32_t pos_x = 0; pos_x < img_width; pos_x++)
            {
                // Calculate source byte and nibble
                uint32_t src_byte_idx = (pos_y * bytes_per_row) + (pos_x / 2);
                bool is_high_nibble = (pos_x % 2 == 0);
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
                if (inverted)
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
                uint32_t dst_x = x + pos_x;
                uint32_t dst_y = y + pos_y;
                uint32_t dst_byte_idx = (dst_y * display_bytes_per_row) + (dst_x / 2);
                bool dst_is_high_nibble = (dst_x % 2 == 0);

                // Write the pixel to the framebuffer
                if (dst_is_high_nibble)
                {
                    framebuffer[dst_byte_idx] = (framebuffer[dst_byte_idx] & 0x0F) | (pixel << 4);
                }
                else
                {
                    framebuffer[dst_byte_idx] = (framebuffer[dst_byte_idx] & 0xF0) | pixel;
                }
            }
        }

        // TODO: ewwies!! maybe fix these bounds bounds calculations...
        // bounds = {
        //     .x = static_cast<int32_t>(x),
        //     .y = static_cast<int32_t>(y),
        //     .width = static_cast<int32_t>(img_width),
        //     .height = static_cast<int32_t>(img_height)};

        bounds = {
            .x = max(0, min(EPD_WIDTH - 1, static_cast<int32_t>(x))),
            .y = max(0, min(EPD_HEIGHT - 1, static_cast<int32_t>(y))),
            .width = min(static_cast<int32_t>(img_width),
                         EPD_WIDTH - max(0, static_cast<int32_t>(x))),
            .height = min(static_cast<int32_t>(img_height),
                          EPD_HEIGHT - max(0, static_cast<int32_t>(y)))};

        free(img_buffer);
        http.end();
        return;
    }

    void clearArea(uint8_t *framebuffer) override
    {
        int16_t cycles = 1;
        int16_t time = 50;

        for (int32_t c = 0; c < cycles; c++)
        {
            for (int32_t i = 0; i < 4; i++)
            {
                epd_push_pixels(bounds, time, 0);
            }
            for (int32_t i = 0; i < 4; i++)
            {
                epd_push_pixels(bounds, time, 1);
            }
        }

        clear_area(bounds, framebuffer);
    }

    bool updateFromJson(JsonObject &element) override
    {
        if (strcmp(element["type"] | "", "image") != 0)
            return false;

        const char *textContent = element["text"] | "";
        const char *callbackContent = element["callback"] | "";
        const char *nameContent = element["name"] | "";
        const char *pathContent = element["path"] | "";

        id = element["id"].as<uint16_t>();
        text = strdup(textContent);
        x = element["x"].as<int16_t>();
        y = element["y"].as<int16_t>();
        anchor = getAnchorFromString(element["anchor"] | "bl");
        props = FontProperties(); // Default, unused
        active = true;
        changed = true;
        updated = true;
        callback = strdup(callbackContent);
        name = strdup(nameContent);
        path = strdup(pathContent);
        touched = false;
        inverted = element["inverted"] | false;
        filled = element["filled"] | true;
        width = element["width"].as<int16_t>();
        height = element["height"].as<int16_t>();
        return true;
    }
};
#endif // IMAGE_ELEMENT_H