#ifndef IMAGE_ELEMENT_H
#define IMAGE_ELEMENT_H

#include "../utils/eink.h"
#include "../utils/layout.h"
#include "../utils/storage.h"
#include "element.h"
#include <HTTPClient.h>
#include <WiFi.h>

enum class ImageType {
    ICON,
    SPOTIFY_ALBUM_ART,
    URL
};

// TODO: Maybe seperate out the image and the icon types?

class ImageElement : public DrawElement {
private:
    // char *text;          // Unused now
    // bool inverted;       // Whether the image is inverted
    char *name;  // The name of the image
    char *path;  // The path of the image, either "icon" or "album-art" or "url"
    bool filled; // Whether the image is filled
    // TODO: Make sure new image wxh tags are updated in this document
    int16_t img_w;      // The width of the image
    int16_t img_h;      // The height of the image
    ImageType img_type; // The type of the image

    // The following are inherited from DrawElement and are here for reference.
    // uint16_t id;
    // char *text;
    // int16_t x;
    // int16_t y;
    // Anchor anchor;
    // FontProperties props;
    // Rect_t bounds;
    // bool active;
    // bool changed;
    // bool updated;

#pragma region(fetch from server / SD)Private Methods
    /**
     * @brief Fetch an image from the server
     */
    // TODO: move elsewhere?
    bool fetchImageFromServer(uint8_t *img_buffer, uint32_t &img_width, uint32_t &img_height) {
        HTTPClient http;
        String url = String(BASE_URL) + "/image/";

        if (strncmp(path, "icon", 4) == 0) {
            url += String(path) + "/" + String(name) +
                   "?width=" + String(width) +
                   "&height=" + String(height) +
                   "&filled=" + String(filled);
        } else if (strncmp(path, "album-art", 9) == 0) {
            url += String(path) + "/" +
                   "?width=" + String(width) +
                   "&height=" + String(height);
        } else {
            Serial.println("Invalid path type");
            return false;
        }

        LOG_D("URL: %s", url.c_str());
        http.setTimeout(10000); // Set timeout to 10 seconds
        http.begin(url);
        int httpCode = http.GET();

        if (httpCode != HTTP_CODE_OK) {
            LOG_E("HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
            http.end();
            return false;
        }

        // Get dimensions from header
        if (http.getStream().readBytes((char *)&img_width, 4) != 4 ||
            http.getStream().readBytes((char *)&img_height, 4) != 4) {
            LOG_E("Failed to read image dimensions");
            http.end();
            return false;
        }

        // Calculate size and read image data
        size_t bytes_per_row = img_width / 2;
        size_t image_data_size = bytes_per_row * img_height;

        if (http.getStream().readBytes((char *)img_buffer, image_data_size) != image_data_size) {
            LOG_E("Failed to read complete image data");
            http.end();
            return false;
        }

        http.end();
        LOG_D("Fetched image from server");
        return true;
    }

    /**
     * @brief Read an image from the SD card
     */
    // TODO: move elsewhere?
    bool readImageFromSD(uint8_t *img_buffer, uint32_t &img_width, uint32_t &img_height) {
        String filename = String("/") + String(width) + "x" + String(height) + "_" + String(name);
        LOG_D("Checking if image exists on SD card: %s", filename.c_str());
        if (!SD.exists(filename)) {
            LOG_D("Image not found on SD card: %s", filename.c_str());
            return false;
        }

        File file = SD.open(filename, FILE_READ);
        if (!file) {
            LOG_E("Failed to open image file");
            return false;
        }

        // Read dimensions
        if (file.read((uint8_t *)&img_width, 4) != 4 ||
            file.read((uint8_t *)&img_height, 4) != 4) {
            file.close();
            LOG_E("Failed to read image dimensions");
            return false;
        }

        // Read image data
        size_t bytes_per_row = img_width / 2;
        size_t image_data_size = bytes_per_row * img_height;
        if (file.read(img_buffer, image_data_size) != image_data_size) {
            file.close();
            LOG_E("Failed to read image data");
            return false;
        }

        file.close();
        LOG_D("Read image from SD card");
        return true;
    }

    /**
     * @brief Get the type of the image from the path
     */
    ImageType getImageTypeFromString(const char *path) {
        if (strncmp(path, "icon", 4) == 0)
            return ImageType::ICON;
        if (strncmp(path, "spotify-album-art", 9) == 0)
            return ImageType::SPOTIFY_ALBUM_ART;
        return ImageType::URL;
    }

    /**
     * @brief Check if the image should be inverted
     */
    bool shouldInvert() {
        // TODO: Implement, check the current background color and do the opposite
        // or use font properties if it's updated
        return false;
    }

#pragma endregion

public:
    // Constructor for ImageElement
    ImageElement() : DrawElement() {
        type = ElementType::IMAGE;
        name = nullptr;
        path = nullptr;
    }

    // destructor
    ~ImageElement() {
        if (name) {
            free(name);
        }
        if (path) {
            free(path);
        }
    }

#pragma region Drawing Methods
    /**
     * @brief Draw the image to the framebuffer
     */
    void draw(uint8_t *framebuffer) override {
        if (WiFi.status() != WL_CONNECTED) {
            LOG_E("WiFi not connected!");
            return;
        }

        uint32_t img_width, img_height;
        size_t bytes_per_row = width / 2;
        size_t image_data_size = bytes_per_row * height;

        uint8_t *img_buffer = (uint8_t *)malloc(image_data_size);
        if (!img_buffer) {
            LOG_E("Failed to allocate image buffer");
            return;
        }

        bool success = false;
        if (isCardMounted()) {
            success = readImageFromSD(img_buffer, img_width, img_height);
        }

        if (!success) {
            success = fetchImageFromServer(img_buffer, img_width, img_height);
            if (success) {
                String filename = String("/") + String(width) + "x" + String(height) + "_" + String(name);
                saveImageToSD(filename, img_buffer, img_width, img_height);
            }
        }

        if (!success) {
            free(img_buffer);
            return;
        }

        // Validate image will fit on display
        if (x + img_width > EPD_WIDTH || y + img_height > EPD_HEIGHT ||
            x < 0 || y < 0) {
            LOG_E("Image position out of bounds");
            free(img_buffer);
            return;
        }

        // Calculate byte offset for x position (2 pixels per byte)
        uint16_t x_byte_offset = x / 2;
        uint8_t x_pixel_offset = x % 2;

        // Copy image data into framebuffer at correct position
        for (uint32_t pos_y = 0; pos_y < img_height; pos_y++) {
            for (uint32_t pos_x = 0; pos_x < img_width; pos_x++) {
                // Calculate source byte and nibble
                uint32_t src_byte_idx = (pos_y * bytes_per_row) + (pos_x / 2);
                bool is_high_nibble = (pos_x % 2 == 0);
                uint8_t pixel;

                // Extract the pixel value (4 bits)
                if (is_high_nibble) {
                    pixel = (img_buffer[src_byte_idx] >> 4) & 0x0F;
                } else {
                    pixel = img_buffer[src_byte_idx] & 0x0F;
                }

                // Handle inversion and transparency
                if (inverted) {
                    if (pixel == 0x0F)
                        pixel = 0x00;
                    else if (pixel == 0x00)
                        pixel = 0x0F;

                    // Skip black pixels for transparency
                    if (pixel == 0x00)
                        continue;
                } else {
                    // skip white pixels for transparency
                    if (pixel == 0x0F)
                        continue;
                }

                // Calculate destination position
                uint32_t dst_x = x + pos_x;
                uint32_t dst_y = y + pos_y;
                uint32_t dst_byte_idx = (dst_y * EPD_WIDTH / 2) + (dst_x / 2);
                bool dst_is_high_nibble = (dst_x % 2 == 0);

                // Write the pixel to the framebuffer
                if (dst_is_high_nibble) {
                    framebuffer[dst_byte_idx] = (framebuffer[dst_byte_idx] & 0x0F) | (pixel << 4);
                } else {
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
        return;
    }

    /**
     * @brief Clear the area of the image
     */
    void clearArea(uint8_t *framebuffer) override {
        int16_t cycles = 1;
        int16_t time = 50;

        for (int32_t c = 0; c < cycles; c++) {
            for (int32_t i = 0; i < 4; i++) {
                epd_push_pixels(bounds, time, 0);
            }
            for (int32_t i = 0; i < 4; i++) {
                epd_push_pixels(bounds, time, 1);
            }
        }

        clear_area(bounds, framebuffer);
    }

    void drawTouched(uint8_t *framebuffer) override {
        // TODO: Implement touch effect for image / icon
        // Maybe get image from SD if exists and turn the non background color pixels to grey
    }

#pragma endregion

    /**
     * @brief Update the image element from a JSON object
     *
     * Example JSON structure:
     * {
     *   "type": "image",
     *   "id": 1,
     *   "text": "",
     *   "x": 100,
     *   "y": 100,
     *   "anchor": "bl",
     *   "callback": "",
     *   "name": "bell",
     *   "path": "icon",
     *   "inverted": false,
     *   "filled": true,
     *   "width": 64,
     *   "height": 64
     * }
     */
    bool updateFromJson(JsonObject &element) override {
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
        font_props = FontProperties(); // Default, unused
        active = true;
        changed = true;
        updated = true;
        callback = strdup(callbackContent);
        name = strdup(nameContent);
        path = strdup(pathContent);
        touched = false;
        filled = element["filled"] | true;
        width = element["width"].as<int16_t>();
        height = element["height"].as<int16_t>();
        img_type = getImageTypeFromString(pathContent);
        return true;
    }

#pragma region isEqual
    bool isEqual(const ImageElement &other) const {
        return DrawElement::isEqual(static_cast<const DrawElement &>(other)) &&
               strcmp(name, other.name) == 0 &&
               strcmp(path, other.path) == 0 &&
               width == other.width &&
               height == other.height &&
               anchor == other.anchor &&
               x == other.x &&
               y == other.y &&
               filled == other.filled;
    }

    // Add this to maintain the override of the base class method
    bool isEqual(const DrawElement &other) const override {
        if (other.getType() != ElementType::IMAGE)
            return false;
        return isEqual(static_cast<const ImageElement &>(other));
    }
#pragma endregion
};
#endif // IMAGE_ELEMENT_H