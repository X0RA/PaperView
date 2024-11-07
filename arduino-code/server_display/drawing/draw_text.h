#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <Arduino.h>
#include "epd_driver.h"
#include "firasans.h"
#include "draw_display.h"
#include <ArduinoJson.h>

// Anchor points
typedef enum
{
    ANCHOR_TR, // Top right
    ANCHOR_TM, // Top middle
    ANCHOR_TL, // Top left
    ANCHOR_BL, // Bottom left
    ANCHOR_BM, // Bottom middle
    ANCHOR_BR, // Bottom right
    ANCHOR_M   // Middle
} TextAnchor_t;

// Text element structure
typedef struct
{
    uint16_t id;
    char *text;
    int16_t x;
    int16_t y;
    TextAnchor_t anchor;
    FontProperties props;
    Rect_t bounds;
    bool active;
    bool changed; // New field to track changes
    bool updated; // New field to track updates
} TextElement_t;

#define MAX_TEXT_ELEMENTS 50
static TextElement_t textElements[MAX_TEXT_ELEMENTS] = {};
static uint16_t elementCount = 0;

// Compare two TextElement_t objects
bool isTextElementEqual(const TextElement_t &a, const TextElement_t &b)
{
    if (a.id != b.id)
        return false;
    if (strcmp(a.text, b.text) != 0)
        return false;
    if (a.x != b.x || a.y != b.y)
        return false;
    if (a.anchor != b.anchor)
        return false;
    if (memcmp(&a.props, &b.props, sizeof(FontProperties)) != 0)
        return false;
    return true;
}

// Find text element by ID
TextElement_t *findTextElement(uint16_t id)
{
    if (elementCount == 0)
    {
        return NULL;
    }

    uint16_t found = 0;
    for (int i = 0; i < MAX_TEXT_ELEMENTS; i++)
    {
        if (textElements[i].active)
        {
            if (textElements[i].id == id)
            {
                return &textElements[i];
            }
            found++;
            if (found >= elementCount)
            {
                break;
            }
        }
    }
    return NULL;
}

// Store text element
bool storeTextElement(const TextElement_t &element)
{
    // Validate input
    if (!element.text)
    {
        Serial.println("Warning: Attempted to store text element with null text");
        return false;
    }

    // First look for existing element with same ID to remove it
    for (int i = 0; i < MAX_TEXT_ELEMENTS; i++)
    {
        if (textElements[i].active && textElements[i].id == element.id)
        {
            // Free the old element's text
            if (textElements[i].text)
            {
                free(textElements[i].text);
            }
            textElements[i].active = false;
            elementCount--;
            break;
        }
    }

    // Find free slot for new element
    for (int i = 0; i < MAX_TEXT_ELEMENTS; i++)
    {
        if (!textElements[i].active)
        {
            // Create a new copy of the text
            char *newText = strdup(element.text);
            if (!newText)
            {
                Serial.println("Error: Failed to allocate memory for text");
                return false;
            }

            // Store the new element
            textElements[i] = element;
            textElements[i].text = newText;
            textElements[i].active = true;
            textElements[i].changed = true;
            textElements[i].updated = true;
            elementCount++;
            return true;
        }
    }

    Serial.println("Warning: No free slots for text element storage!");
    return false;
}

// Convert string anchor to enum
TextAnchor_t getAnchorFromString(const char *anchor)
{
    if (strcmp(anchor, "tr") == 0)
        return ANCHOR_TR;
    if (strcmp(anchor, "tm") == 0)
        return ANCHOR_TM;
    if (strcmp(anchor, "tl") == 0)
        return ANCHOR_TL;
    if (strcmp(anchor, "bl") == 0)
        return ANCHOR_BL;
    if (strcmp(anchor, "bm") == 0)
        return ANCHOR_BM;
    if (strcmp(anchor, "br") == 0)
        return ANCHOR_BR;
    if (strcmp(anchor, "m") == 0)
        return ANCHOR_M;
    return ANCHOR_TL; // Default to top left
}

// Draw text and update TextElement with calculated bounds
void drawText(TextElement_t *element, uint8_t *framebuffer)
{
    if (!element || !element->text)
    {
        Serial.println("Error: Invalid text element or null text");
        return;
    }

    int32_t x1, y1, w, h;

    // First calculate bounds without modifying any positions
    int32_t temp_x = element->x;
    int32_t temp_y = element->y;

    get_text_bounds((GFXfont *)&FiraSans, element->text,
                    &temp_x, &temp_y,
                    &x1, &y1, &w, &h, NULL);

    // Calculate final cursor position based on anchor and bounds
    int32_t cursor_x = element->x;
    int32_t cursor_y = element->y;

    // Calculate position based on anchor and text dimensions
    switch (element->anchor)
    {
    case ANCHOR_TR:
        cursor_x = element->x - w;
        cursor_y = element->y;
        break;
    case ANCHOR_TM:
        cursor_x = element->x - (w / 2);
        cursor_y = element->y;
        break;
    case ANCHOR_TL:
        cursor_x = element->x;
        cursor_y = element->y;
        break;
    case ANCHOR_BL:
        cursor_x = element->x;
        cursor_y = element->y + h;
        break;
    case ANCHOR_BM:
        cursor_x = element->x - (w / 2);
        cursor_y = element->y + h;
        break;
    case ANCHOR_BR:
        cursor_x = element->x - w;
        cursor_y = element->y + h;
        break;
    case ANCHOR_M:
        cursor_x = element->x - (w / 2);
        cursor_y = element->y + (h / 2);
        break;
    }

    Rect_t newBounds = {
        .x = max(0, min(EPD_WIDTH - 1, cursor_x)),
        .y = max(0, min(EPD_HEIGHT - 1, cursor_y - h)),
        .width = min(w, EPD_WIDTH - newBounds.x),
        .height = min(h, EPD_HEIGHT - newBounds.y)};

    // Additional check to ensure width/height aren't negative
    if (newBounds.width < 0)
        newBounds.width = 0;
    if (newBounds.height < 0)
        newBounds.height = 0;

    // Draw the text
    Serial.printf("Drawing text: '%s' at (%d,%d)\n",
                  element->text, cursor_x, cursor_y);

    write_mode((GFXfont *)&FiraSans, element->text,
               &cursor_x, &cursor_y,
               framebuffer,
               BLACK_ON_WHITE,
               &element->props);

    // Update element with new bounds
    element->bounds = newBounds;
    element->active = true;
}

// Clear a text element's area from the display and framebuffer
void clearTextElementArea(TextElement_t *element, uint8_t *framebuffer)
{
    if (!element)
        return;

    const int32_t padding_x = 8;
    int32_t padding_y = 6;
    const char tallerChars[5] = {'g', 'j', 'p', 'q', 'y'};
    for (int i = 0; i < 5; i++)
    {
        if (strchr(element->text, tallerChars[i]) != NULL)
        {
            padding_y += 3;
            break;
        }
    }
    Rect_t clearArea = {
        .x = max(0, min(EPD_WIDTH - 1, element->bounds.x - padding_x)),
        .y = max(0, min(EPD_HEIGHT - 1, element->bounds.y - padding_y)),
        .width = min(element->bounds.width + (padding_x * 2),
                     EPD_WIDTH - max(0, element->bounds.x - padding_x)),
        .height = min(element->bounds.height + (padding_y * 2),
                      EPD_HEIGHT - max(0, element->bounds.y - padding_y))};

    // Additional check to ensure width/height aren't negative
    if (clearArea.width < 0)
        clearArea.width = 0;
    if (clearArea.height < 0)
        clearArea.height = 0;

    // Debug print
    Serial.printf("Clear area after clipping: x=%d, y=%d, w=%d, h=%d\n",
                  clearArea.x, clearArea.y, clearArea.width, clearArea.height);

    // Update elements area live

    // epd_clear_area(clearArea);

    // uint8_t bg_color = current_display.background_color == 15 ? 1 : 0;
    // uint8_t text_color = current_display.foreground_color == 15 ? 1 : 0;
    int16_t cycles = 1;
    int16_t time = 50;

    // Push pixels with appropriate timing
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

    // Clear the framebuffer area
    clear_area(clearArea, framebuffer);

    Serial.printf("Cleared text area for ID %d at (%d,%d,%d,%d)\n",
                  element->id,
                  clearArea.x, clearArea.y,
                  clearArea.width, clearArea.height);
}

void clearTextElements()
{
    for (int i = 0; i < MAX_TEXT_ELEMENTS; i++)
    {
        if (textElements[i].active && textElements[i].text)
        {
            free(textElements[i].text);
        }
    }
    memset(textElements, 0, sizeof(textElements));
    elementCount = 0;
}

// Process text elements from JSON
void processTextElements(JsonArray &elements, uint8_t *framebuffer)
{
    // Mark all existing elements as not updated
    for (int i = 0; i < MAX_TEXT_ELEMENTS; i++)
    {
        if (textElements[i].active)
        {
            textElements[i].updated = false;
            textElements[i].changed = false;
        }
    }

    // Process new elements
    for (JsonObject element : elements)
    {
        const char *type = element["type"] | "";
        if (strcmp(type, "text") != 0)
        {
            continue;
        }

        // Get text content first to check if valid
        const char *textContent = element["text"] | "";
        if (strlen(textContent) == 0)
        {
            Serial.println("Warning: Empty text field skipped");
            continue;
        }

        TextElement_t newTextElement = {
            .id = element["id"].as<uint16_t>(),
            .text = strdup(textContent),
            .x = element["x"].as<int16_t>(),
            .y = element["y"].as<int16_t>(),
            .anchor = getAnchorFromString(element["anchor"] | "bl"),
            .props = get_text_properties(element["level"].as<uint8_t>()),
            .active = true,
            .changed = true,
            .updated = true};

        // Find existing element
        TextElement_t *existingElement = findTextElement(newTextElement.id);
        if (existingElement != NULL)
        {
            // Compare newTextElement with existingElement
            if (isTextElementEqual(*existingElement, newTextElement))
            {
                // clearTextElementArea(existingElement, framebuffer);
                // Elements are the same
                existingElement->updated = true;
                existingElement->changed = false;
                // Free the newTextElement.text since we don't need it
                free(newTextElement.text);
            }
            else
            {
                // Elements are different
                // Clear old area
                clearTextElementArea(existingElement, framebuffer);

                // Free old text
                free(existingElement->text);

                // Update existing element with new data
                *existingElement = newTextElement;
                existingElement->changed = true;
                existingElement->updated = true;
            }
        }
        else
        {
            storeTextElement(newTextElement);
            drawText(findTextElement(newTextElement.id), framebuffer);
            clearTextElementArea(findTextElement(newTextElement.id), framebuffer);
        }
    }

    // Remove elements that are no longer present
    for (int i = 0; i < MAX_TEXT_ELEMENTS; i++)
    {
        if (textElements[i].active && !textElements[i].updated)
        {
            // Element was not updated, so it's removed
            // Clear its area
            clearTextElementArea(&textElements[i], framebuffer);

            // Free text
            if (textElements[i].text)
            {
                free(textElements[i].text);
            }
            // Mark as inactive
            textElements[i].active = false;
            elementCount--;
        }
    }

    // Re-draw all active elements to buffer
    for (int i = 0; i < MAX_TEXT_ELEMENTS; i++)
    {
        if (textElements[i].active)
        {
            // If element changed, update display area
            if (textElements[i].changed)
            {
                uint8_t text_color = textElements[i].props.fg_color & 0x0F;
                // Update display area
                epd_push_pixels(textElements[i].bounds, 50, 1);
            }

            // Re-draw text to buffer
            drawText(&textElements[i], framebuffer);
        }
    }
}

#endif // TEXT_UTILS_H
