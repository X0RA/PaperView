#ifndef DRAW_ELEMENT_H
#define DRAW_ELEMENT_H

#include "../managers/display_manager.h"
#include <Arduino.h>
#include "epd_driver.h"
#include "firasans.h"
#include <ArduinoJson.h>

enum class ElementType {
    TEXT,
    BUTTON,
    IMAGE
};

enum class Anchor
{
    TOP_RIGHT,
    TOP_MIDDLE,
    TOP_LEFT,
    BOTTOM_LEFT,
    BOTTOM_MIDDLE,
    BOTTOM_RIGHT,
    MIDDLE
};

class DrawElement
{
protected:
    ElementType type;
    uint16_t id;
    char *text;
    int16_t x;
    int16_t y;
    Anchor anchor;
    FontProperties props;
    Rect_t bounds;
    bool active;
    bool changed;
    bool updated;

    static Anchor getAnchorFromString(const char *anchor)
    {
        if (strcmp(anchor, "tr") == 0)
            return Anchor::TOP_RIGHT;
        if (strcmp(anchor, "tm") == 0)
            return Anchor::TOP_MIDDLE;
        if (strcmp(anchor, "tl") == 0)
            return Anchor::TOP_LEFT;
        if (strcmp(anchor, "bl") == 0)
            return Anchor::BOTTOM_LEFT;
        if (strcmp(anchor, "bm") == 0)
            return Anchor::BOTTOM_MIDDLE;
        if (strcmp(anchor, "br") == 0)
            return Anchor::BOTTOM_RIGHT;
        if (strcmp(anchor, "m") == 0)
            return Anchor::MIDDLE;
        return Anchor::TOP_LEFT;
    }

public:
    DrawElement() : id(0), text(nullptr), x(0), y(0), active(false), changed(false), updated(false) {}

    virtual ~DrawElement()
    {
        if (text)
        {
            free(text);
        }
    }

    ElementType getType() const { return type; }
    virtual void draw(uint8_t *framebuffer) = 0;
    virtual void clearArea(uint8_t *framebuffer)
    {
        Serial.printf("Clearing element with id %d\n", id);
        const int32_t padding_x = 8;
        int32_t padding_y = 6;
        const char tallerChars[5] = {'g', 'j', 'p', 'q', 'y'};
        for (int i = 0; i < 5; i++)
        {
            if (strchr(text, tallerChars[i]) != NULL)
            {
                padding_y += 3;
                break;
            }
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

        int16_t cycles = 1;
        int16_t time = 50;

        for (int32_t c = 0; c < cycles; c++)
        {
            for (int32_t i = 0; i < 4; i++)
            {
                epd_push_pixels(clearArea, time, 0);
            }
            for (int32_t i = 0; i < 4; i++)
            {
                epd_push_pixels(clearArea, time, 1);
            }
        }

        clear_area(clearArea, framebuffer);

        Serial.printf("Cleared text area for ID %d at (%d,%d,%d,%d)\n",
                      id,
                      clearArea.x, clearArea.y,
                      clearArea.width, clearArea.height);
    }
    virtual bool updateFromJson(JsonObject &element) = 0;

    bool isEqual(const DrawElement &other) const
    {
        return (id == other.id &&
                strcmp(text, other.text) == 0 &&
                x == other.x &&
                y == other.y &&
                anchor == other.anchor);
        // &&memcmp(&props, &other.props, sizeof(FontProperties)) == 0);
    }

    String getText() const { return String(text); }
    uint16_t getId() const { return id; }
    bool isActive() const { return active; }
    void setUpdated(bool value) { updated = value; }
    bool isUpdated() const { return updated; }
    const Rect_t &getBounds() const { return bounds; }
};

#endif // DRAW_ELEMENT_H