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
    // char *callback  //callback url
    char *name;         // The name of the image
    char *endpoint;     // The endpoint of the image, either "icon" or "album-art" or "url"
    bool filled;        // Whether the image is filled
    int16_t width;      // The width of the image
    int16_t height;     // The height of the image
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
     * @brief Get the type of the image from the endpoint
     */
    ImageType getImageTypeFromString(const char *endpoint) {
        if (strncmp(endpoint, "icon", 4) == 0)
            return ImageType::ICON;
        if (strncmp(endpoint, "album-art", 9) == 0)
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
        endpoint = nullptr;
    }

    // destructor
    ~ImageElement() {
        if (name) {
            free(name);
        }
        if (endpoint) {
            free(endpoint);
        }
    }

    void updateElement() override {
        // TODO: Implement
        return;
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

        // Not really used but keeping for now
        uint32_t received_width, received_height;
        size_t bytes_per_row = width / 2;
        size_t image_data_size = bytes_per_row * height;

        String storage_filename = String(name) + "_" + String(width) + "x" + String(height) + ".bin";

        String url;

        // routes are /album-art and /icon/<icon_name>
        // params are width, height, and filled for icons
        switch (img_type) {
        case ImageType::ICON:
            url = String(BASE_URL) + "/image/" + String(endpoint) + String("/") + String(name) + "?width=" + String(width) + "&height=" + String(height) + "&filled=" + String(filled);
            break;
        case ImageType::SPOTIFY_ALBUM_ART:
            url = String(BASE_URL) + "/image/" + String(endpoint) + String("/") + "?width=" + String(width) + "&height=" + String(height);
            break;
        // unused as of now
        case ImageType::URL:
            url = String(BASE_URL) + "/image/" + String(endpoint) + "/" + String(name) + "?width=" + String(width) + "&height=" + String(height);
            break;
        }

        uint8_t *img_buffer = (uint8_t *)malloc(image_data_size);
        if (!img_buffer) {
            LOG_E("Failed to allocate image buffer");
            return;
        }

        bool success = false;
        if (isCardMounted()) {
            success = readImageBufferFromSD(storage_filename, img_buffer, received_width, received_height);
        }

        if (!success) {
            // TODO: ewwies what am i doing here?
            success = fetchImageFromServer(url, img_buffer, received_width, received_height,
                                           static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            if (success) {
                saveImageBufferToSD(storage_filename, img_buffer, received_width, received_height);
            }
        }

        if (!success) {
            free(img_buffer);
            return;
        }

        // Validate image will fit on display
        if (x + width > EPD_WIDTH || y + height > EPD_HEIGHT ||
            x < 0 || y < 0) {
            LOG_E("Image position out of bounds");
            free(img_buffer);
            return;
        }

        // Calculate byte offset for x position (2 pixels per byte)
        uint16_t x_byte_offset = x / 2;
        uint8_t x_pixel_offset = x % 2;

        // Copy image data into framebuffer at correct position
        for (uint32_t pos_y = 0; pos_y < height; pos_y++) {
            for (uint32_t pos_x = 0; pos_x < width; pos_x++) {
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
                if (shouldInvert()) {
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
        //     .width = static_cast<int32_t>(width),
        //     .height = static_cast<int32_t>(height)};

        bounds = {
            .x = max(0, min(EPD_WIDTH - 1, static_cast<int32_t>(x))),
            .y = max(0, min(EPD_HEIGHT - 1, static_cast<int32_t>(y))),
            .width = min(static_cast<int32_t>(width),
                         EPD_WIDTH - max(0, static_cast<int32_t>(x))),
            .height = min(static_cast<int32_t>(height),
                          EPD_HEIGHT - max(0, static_cast<int32_t>(y)))};

        free(img_buffer);
        return;
    }

    // TODO: Implement touch effect for image / icon
    // Maybe get image from SD if exists and turn the non background color pixels to grey
    void drawTouched(uint8_t *framebuffer) override {
        if (img_type == ImageType::ICON) {
            String storage_filename = String(name) + "_" + String(width) + "x" + String(height) + ".bin";
            // Not really used but keeping for now
            uint32_t received_width, received_height;
            size_t bytes_per_row = width / 2;
            size_t image_data_size = bytes_per_row * height;
            uint8_t *img_buffer = (uint8_t *)malloc(image_data_size);
            if (!img_buffer) {
                LOG_E("Failed to allocate image buffer");
                return;
            }
            bool success = false;
            if (isCardMounted()) {
                success = readImageBufferFromSD(storage_filename, img_buffer, received_width, received_height);
            }
            if (!success) {
                free(img_buffer);
                return;
            }
            // Calculate byte offset for x position (2 pixels per byte)
            uint16_t x_byte_offset = x / 2;
            uint8_t x_pixel_offset = x % 2;

            // Copy image data into framebuffer at correct position
            for (uint32_t pos_y = 0; pos_y < height; pos_y++) {
                for (uint32_t pos_x = 0; pos_x < width; pos_x++) {
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
                    if (pixel == 0x0F)
                        pixel = 0x00;
                    else if (pixel == 0x00)
                        pixel = 0x0F;

                    // Skip black pixels for transparency
                    if (pixel == 0x00)
                        continue;

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
        }
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
     *   "endpoint": "icon",
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
        const char *endpointContent = element["endpoint"] | "";

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
        endpoint = strdup(endpointContent);
        touched = false;
        filled = element["filled"] | true;
        width = element["width"].as<int16_t>();
        height = element["height"].as<int16_t>();
        img_type = getImageTypeFromString(endpointContent);
        return true;
    }

#pragma region isEqual
    bool isEqual(const ImageElement &other) const {
        return DrawElement::isEqual(static_cast<const DrawElement &>(other)) &&
               strcmp(name, other.name) == 0 &&
               strcmp(endpoint, other.endpoint) == 0 &&
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