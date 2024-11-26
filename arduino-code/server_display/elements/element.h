#ifndef DRAW_ELEMENT_H
#define DRAW_ELEMENT_H

#include "../utils/eink.h"
#include "epd_driver.h"
#include "firasans.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../utils/http.h"
#include "../utils/types.h"

// Forward declare ImageType enum
enum class ImageType {
    ICON,
    SPOTIFY_ALBUM_ART,
    URL
};

enum class ElementType {
    TEXT,
    BUTTON,
    IMAGE
};

enum class Anchor {
    TOP_RIGHT,
    TOP_MIDDLE,
    TOP_LEFT,
    BOTTOM_LEFT,
    BOTTOM_MIDDLE,
    BOTTOM_RIGHT,
    MIDDLE_LEFT,
    MIDDLE,
    MIDDLE_RIGHT
};

class DrawElement {

#pragma region properties
protected:
    ElementType type;          // The type of the element
    uint16_t id;               // The id of the element
    char *text;                // The text to display on the element
    char *callback;            // The callback, url or function name to call when the element is touched
    int16_t x;                 // The x position of the element
    int16_t y;                 // The y position of the element
    Anchor anchor;             // The anchor of the element
    Rect_t bounds;             // The bounds of the element
    FontProperties font_props; // The properties of the font
    bool touched;              // Indicates if the element was touched in current update cycle
    RefreshType refresh_type;  // Current type of refresh to perform on the element
#pragma endregion

    static Anchor getAnchorFromString(const char *anchor) {
        if (strcmp(anchor, "tl") == 0)
            return Anchor::TOP_LEFT;
        if (strcmp(anchor, "tm") == 0)
            return Anchor::TOP_MIDDLE;
        if (strcmp(anchor, "tr") == 0)
            return Anchor::TOP_RIGHT;
        if (strcmp(anchor, "ml") == 0)
            return Anchor::MIDDLE_LEFT;
        if (strcmp(anchor, "m") == 0)
            return Anchor::MIDDLE;
        if (strcmp(anchor, "mr") == 0)
            return Anchor::MIDDLE_RIGHT;
        if (strcmp(anchor, "bl") == 0)
            return Anchor::BOTTOM_LEFT;
        if (strcmp(anchor, "bm") == 0)
            return Anchor::BOTTOM_MIDDLE;
        if (strcmp(anchor, "br") == 0)
            return Anchor::BOTTOM_RIGHT;
        return Anchor::TOP_LEFT;
    }

public:
    DrawElement() : id(0), text(nullptr), x(0), y(0), callback(nullptr), touched(false) {}

    // destructor
    virtual ~DrawElement() {
        if (text) {
            free(text);
        }
        if (callback) {
            free(callback);
        }
    }

#pragma region Virtual Methods not defined in the base class
    /**
     * @brief Draw the element to the framebuffer
     * @param framebuffer The framebuffer to draw to
     */
    // TODO: This should update the active bool?
    virtual void draw(uint8_t *framebuffer) = 0;

    /**
     * @brief Update the element from a JSON object
     * @param element The JSON object to update from
     * @return Whether the update was successful
     */
    virtual bool updateFromJson(JsonObject &element) = 0;

    /**
     * @brief Draw the element when it is touched
     * @param framebuffer The framebuffer to draw to
     */
    virtual void drawTouched(uint8_t *framebuffer) = 0; // TODO: Implement these

    // TODO: Implement this
    // This should calculate the bounds of the element based on the text, font size, etc.
    // This should also update the font properties if they have changed (based on background)
    // Should work out if this changes updated / changed / both
    virtual void updateElement() = 0;

    // TODO: Sorta messy implementation, but it works for now. Would preffer to have a nicer way that isn't within element class
    virtual ImageType getImageType() const { return ImageType::ICON; } // Default implementation

#pragma endregion

#pragma region(clearArea, isEqual) Virtual Methods defined in the base class

    /**
     * @brief Clear the area of the element
     * @param framebuffer The framebuffer to clear
     */
    virtual void clearArea(uint8_t *framebuffer, bool framebufferOnly = false) {
        LOG_D("Clearing element with id %d", id);
        int32_t padding_x = 0;
        int32_t padding_y = 0;

        // If the element is a text element, we need to add extra padding for taller characters
        if (type == ElementType::TEXT) {
            padding_x = 8;
            padding_y = 6;
            const char tallerChars[5] = {'g', 'j', 'p', 'q', 'y'};
            for (int i = 0; i < 5; i++) {
                if (strchr(text, tallerChars[i]) != NULL) {
                    padding_y += 3;
                    break;
                }
            }
        }

        if (type == ElementType::IMAGE) {
            if (getImageType() == ImageType::ICON) {
                padding_x = 5;
                padding_y = 5;
            } else {
                padding_x = 9;
                padding_y = 9;
            }
        }

        if (type == ElementType::BUTTON) {
            padding_x = 8;
            padding_y = 8;
        }

        Rect_t clearArea = {
            .x = max(0, min(EPD_WIDTH - 1, bounds.x - padding_x)),
            .y = max(0, min(EPD_HEIGHT - 1, bounds.y - padding_y)),
            .width = min(bounds.width + (padding_x * 2),
                         EPD_WIDTH - max(0, bounds.x - padding_x)),
            .height = min(bounds.height + (padding_y * 2),
                          EPD_HEIGHT - max(0, bounds.y - padding_y))};

        if (clearArea.width < 0)
            clearArea.width = 0;
        if (clearArea.height < 0)
            clearArea.height = 0;
        if (clearArea.x >= EPD_WIDTH)
            clearArea.x = EPD_WIDTH;
        if (clearArea.y >= EPD_HEIGHT)
            clearArea.y = EPD_HEIGHT;

        // Default refresh type is 2 cycles (ELEMENT_REFRESH_PARTIAL)
        int32_t cycles = 2;
        int16_t white_time = 50;
        int16_t dark_time = 50;

        if (refresh_type == RefreshType::ELEMENT_REFRESH_FAST) {
            cycles = 1;
            white_time = 50;
            dark_time = 50;
        }

        // This is the default so no need to list
        // if (refresh_type == RefreshType::SOFT_REFRESH)

        if (refresh_type == RefreshType::ELEMENT_REFRESH_COMPLETE) {
            cycles = 4;
        }

        // clear the framebuffer
        clear_framebuffer_area(clearArea, framebuffer);

        if (framebufferOnly) {
            LOG_D("Cleared framebuffer for element id %d", id);
            return;
        }

        // clear the display
        clear_area(clearArea, framebuffer, cycles, white_time, dark_time);

        LOG_D("Cleared text area for ID %d at (%d,%d,%d,%d)",
              id,
              clearArea.x, clearArea.y,
              clearArea.width, clearArea.height);
    }

    /**
     * @brief Check if the element is equal to another element
     * @param other The element to compare to
     * @return Whether the elements are equal
     */
    virtual bool isEqual(const DrawElement &other) const {
        // Check ID first as it's the primary identifier
        if (id != other.id)
            return false;

        // Handle text comparison with null checks
        if (text == nullptr && other.text == nullptr) {
            // Both null is considered equal
        } else if (text == nullptr || other.text == nullptr) {
            return false; // One null, one not null
        } else if (strcmp(text, other.text) != 0) {
            return false;
        }

        // Handle callback comparison with null checks
        if (callback == nullptr && other.callback == nullptr) {
            // Both null is considered equal
        } else if (callback == nullptr || other.callback == nullptr) {
            return false; // One null, one not null
        } else if (strcmp(callback, other.callback) != 0) {
            return false;
        }

        // Check position and properties
        return x == other.x &&
               y == other.y &&
               anchor == other.anchor &&
               type == other.type;
    }
#pragma endregion

#pragma region touch methods

    bool executeCallback(uint8_t *framebuffer) {
        if (!callback || strlen(callback) == 0) {
            return false;
        }

        // Handle built-in callbacks
        if (strcmp(callback, "toggle-dark") == 0) {
            if (current_display.background_color == 15) {
                set_black_display_mode();
            } else {
                set_white_display_mode();
            }
            return true;
        }

        return makeQuickPost(callback);
    }

    virtual bool isPointInside(int16_t px, int16_t py) const {
        return (px >= bounds.x && px < (bounds.x + bounds.width) &&
                py >= bounds.y && py < (bounds.y + bounds.height));
    }

#pragma endregion

#pragma region Getters and Setters
    // getters
    ElementType getType() const { return type; }
    String getText() const { return String(text); }
    uint16_t getId() const { return id; }
    RefreshType getRefreshType() const { return refresh_type; }
    bool isTouched() const { return touched; }

    // setters
    void setTouched(bool value) { touched = value; }
    void setRefreshType(RefreshType value) { refresh_type = value; }
    const Rect_t &getBounds() const { return bounds; }
#pragma endregion
};

#endif // DRAW_ELEMENT_H