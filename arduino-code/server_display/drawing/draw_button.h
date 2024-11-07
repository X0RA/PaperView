#ifndef DRAW_BUTTON_H
#define DRAW_BUTTON_H

#include <Arduino.h>
#include "epd_driver.h"
#include "firasans.h"
#include "draw_display.h"
#include <ArduinoJson.h>
#include "../utils/api_functions.h"

// Anchor points
typedef enum
{
    B_ANCHOR_TR, // Top right
    B_ANCHOR_TM, // Top middle
    B_ANCHOR_TL, // Top left
    B_ANCHOR_BL, // Bottom left
    B_ANCHOR_BM, // Bottom middle
    B_ANCHOR_BR, // Bottom right
    B_ANCHOR_M   // Middle
} ButtonAnchor_t;

// Button element structure
typedef struct
{
    uint16_t id;
    char *text;
    int16_t x;
    int16_t y;
    ButtonAnchor_t anchor;
    FontProperties props;
    char *callback;
    int16_t padding_x;
    int16_t padding_y;
    uint16_t radius;
    bool filled;
    bool active;
    bool changed;
    bool updated;
    Rect_t bounds;
} ButtonElement_t;

#define MAX_BUTTON_ELEMENTS 50
static ButtonElement_t buttonElements[MAX_BUTTON_ELEMENTS] = {};
static uint16_t buttonElementCount = 0;

void draw_rounded_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t r, uint8_t color, uint8_t *framebuffer)
{
    // Ensure radius doesn't exceed half of width/height
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;

    // Draw horizontal lines
    epd_draw_hline(x + r, y, w - 2 * r, color, framebuffer);         // Top
    epd_draw_hline(x + r, y + h - 1, w - 2 * r, color, framebuffer); // Bottom

    // Draw vertical lines
    epd_draw_vline(x, y + r, h - 2 * r, color, framebuffer);         // Left
    epd_draw_vline(x + w - 1, y + r, h - 2 * r, color, framebuffer); // Right

    // Draw four corners
    epd_draw_circle(x + r, y + r, r, color, framebuffer);                 // Top-left
    epd_draw_circle(x + w - r - 1, y + r, r, color, framebuffer);         // Top-right
    epd_draw_circle(x + r, y + h - r - 1, r, color, framebuffer);         // Bottom-left
    epd_draw_circle(x + w - r - 1, y + h - r - 1, r, color, framebuffer); // Bottom-right
}

void epd_draw_filled_rounded_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                                  uint16_t r, uint8_t color, uint8_t *framebuffer)
{
    // Ensure radius doesn't exceed half of width/height
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;

    // Draw main rectangle body
    epd_fill_rect(x + r, y, w - 2 * r, h, color, framebuffer);

    // Draw side rectangles
    epd_fill_rect(x, y + r, r, h - 2 * r, color, framebuffer);         // Left
    epd_fill_rect(x + w - r, y + r, r, h - 2 * r, color, framebuffer); // Right

    // Draw four corners with filled circles
    epd_fill_circle(x + r, y + r, r, color, framebuffer);                 // Top-left
    epd_fill_circle(x + w - r - 1, y + r, r, color, framebuffer);         // Top-right
    epd_fill_circle(x + r, y + h - r - 1, r, color, framebuffer);         // Bottom-left
    epd_fill_circle(x + w - r - 1, y + h - r - 1, r, color, framebuffer); // Bottom-right
}

// Compare two ButtonElement_t objects
bool isButtonElementEqual(const ButtonElement_t &a, const ButtonElement_t &b)
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
    if (a.radius != b.radius)
        return false;
    return true;
}

// Find button element by ID
ButtonElement_t *findButtonElement(uint16_t id)
{
    if (buttonElementCount == 0)
    {
        return NULL;
    }

    uint16_t found = 0;
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (buttonElements[i].active)
        {
            if (buttonElements[i].id == id)
            {
                return &buttonElements[i];
            }
            found++;
            if (found >= buttonElementCount)
            {
                break;
            }
        }
    }
    return NULL;
}

// Store button element
bool storeButtonElement(const ButtonElement_t &element)
{
    // Validate input
    if (!element.text)
    {
        Serial.println("Warning: Attempted to store button element with null text");
        return false;
    }

    // First look for existing element with same ID to remove it
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (buttonElements[i].active && buttonElements[i].id == element.id)
        {
            // Free the old element's text
            if (buttonElements[i].text)
            {
                free(buttonElements[i].text);
                free(buttonElements[i].callback);
            }
            buttonElements[i].active = false;
            buttonElementCount--;
            break;
        }
    }

    // Find free slot for new element
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (!buttonElements[i].active)
        {
            // Create a new copy of the text
            char *newText = strdup(element.text);
            if (!newText)
            {
                Serial.println("Error: Failed to allocate memory for text");
                return false;
            }

            // Store the new element
            buttonElements[i] = element;
            buttonElements[i].text = newText;
            buttonElements[i].active = true;
            buttonElements[i].changed = true;
            buttonElements[i].updated = true;
            buttonElementCount++;
            return true;
        }
    }

    Serial.println("Warning: No free slots for button element storage!");
    return false;
}

// Convert string anchor to enum
ButtonAnchor_t getButtonAnchorFromString(const char *anchor)
{
    if (strcmp(anchor, "tr") == 0)
        return B_ANCHOR_TR;
    if (strcmp(anchor, "tm") == 0)
        return B_ANCHOR_TM;
    if (strcmp(anchor, "tl") == 0)
        return B_ANCHOR_TL;
    if (strcmp(anchor, "bl") == 0)
        return B_ANCHOR_BL;
    if (strcmp(anchor, "bm") == 0)
        return B_ANCHOR_BM;
    if (strcmp(anchor, "br") == 0)
        return B_ANCHOR_BR;
    if (strcmp(anchor, "m") == 0)
        return B_ANCHOR_M;
    return B_ANCHOR_TL; // Default to top left
}

// Draw button and update ButtonElement with calculated bounds
void drawButton(ButtonElement_t *element, uint8_t *framebuffer, bool touched = false)
{
    if (!element || !element->text)
    {
        Serial.println("Error: Invalid button element or null text");
        return;
    }

    int32_t x1, y1, w, h;

    // First calculate text bounds without modifying any positions
    int32_t temp_x = element->x;
    int32_t temp_y = element->y;

    get_text_bounds((GFXfont *)&FiraSans, element->text,
                    &temp_x, &temp_y,
                    &x1, &y1, &w, &h, NULL);

    // Calculate button dimensions including padding
    int32_t buttonWidth = w + (element->padding_x * 2);
    int32_t buttonHeight = h + (element->padding_y * 2);

    // Calculate button position based on anchor
    int32_t button_x = element->x;
    int32_t button_y = element->y;

    switch (element->anchor)
    {
    case B_ANCHOR_TR:
        button_x = element->x - buttonWidth;
        button_y = element->y;
        break;
    case B_ANCHOR_TM:
        button_x = element->x - (buttonWidth / 2);
        button_y = element->y;
        break;
    case B_ANCHOR_TL:
        button_x = element->x;
        button_y = element->y;
        break;
    case B_ANCHOR_BL:
        button_x = element->x;
        button_y = element->y - buttonHeight;
        break;
    case B_ANCHOR_BM:
        button_x = element->x - (buttonWidth / 2);
        button_y = element->y - buttonHeight;
        break;
    case B_ANCHOR_BR:
        button_x = element->x - buttonWidth;
        button_y = element->y - buttonHeight;
        break;
    case B_ANCHOR_M:
        button_x = element->x - (buttonWidth / 2);
        button_y = element->y - (buttonHeight / 2);
        break;
    }

    // Store the bounds for future clearing
    Rect_t newBounds = {
        .x = button_x,
        .y = button_y,
        .width = buttonWidth,
        .height = buttonHeight};

    uint8_t background_color;
    if (touched)
    {
        background_color = element->props.bg_color == 0 ? 15 : 255;
    }
    else
    {
        background_color = element->props.bg_color == 15 ? 0 : 255;
    }

    if (element->filled)
    {
        epd_draw_filled_rounded_rect(button_x, button_y, buttonWidth, buttonHeight,
                                     element->radius, background_color, framebuffer);
    }
    else
    {
        draw_rounded_rect(button_x, button_y, buttonWidth, buttonHeight,
                          element->radius, background_color, framebuffer);
    }

    // Calculate text position inside the button
    int32_t text_x = button_x + element->padding_x;
    int32_t text_y = button_y + element->padding_y + h;

    // Draw the text
    Serial.printf("Drawing button text: '%s' at (%d,%d)\n",
                  element->text, text_x, text_y);

    FontProperties props = element->props;

    if (element->filled)
    {
        props.bg_color = element->props.fg_color;
        props.fg_color = element->props.bg_color;
    }

    write_mode((GFXfont *)&FiraSans, element->text,
               &text_x, &text_y,
               framebuffer,
               BLACK_ON_WHITE,
               &props);

    // Update element with new bounds
    element->bounds = newBounds;
    element->active = true;
}

// Clear a button element's area from the display and framebuffer
void clearButtonElementArea(ButtonElement_t *element, uint8_t *framebuffer)
{
    if (!element)
        return;

    const int32_t padding_x = 8;
    int32_t padding_y = 6;

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

    // Update elements area live
    uint8_t bg_color = current_display.background_color & 0x0F;
    int16_t time = 50;
    int16_t cycles = 1;

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

    Serial.printf("Cleared button area for ID %d at (%d,%d,%d,%d)\n",
                  element->id,
                  clearArea.x, clearArea.y,
                  clearArea.width, clearArea.height);
}

void clearButtonElements()
{
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (buttonElements[i].active && buttonElements[i].text)
        {
            free(buttonElements[i].text);
            free(buttonElements[i].callback);
        }
    }
    memset(buttonElements, 0, sizeof(buttonElements));
    buttonElementCount = 0;
}

void processButtonElements(JsonArray &elements, uint8_t *framebuffer)
{
    // Mark all existing elements as not updated
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (buttonElements[i].active)
        {
            buttonElements[i].updated = false;
            buttonElements[i].changed = false;
        }
    }

    // Process new elements
    for (JsonObject element : elements)
    {
        const char *type = element["type"] | "";
        if (strcmp(type, "button") != 0)
        {
            continue;
        }

        // Get text content first to check if valid
        const char *textContent = element["text"] | "";
        const char *callbackContent = element["callback"] | "";
        if (strlen(textContent) == 0)
        {
            Serial.println("Warning: Empty text field skipped");
            continue;
        }

        ButtonElement_t newButtonElement = {
            .id = element["id"].as<uint16_t>(),
            .text = strlen(textContent) > 0 ? strdup(textContent) : strdup(""),
            .x = element["x"].as<int16_t>(),
            .y = element["y"].as<int16_t>(),
            .anchor = getButtonAnchorFromString(element["anchor"] | "bl"),
            .props = get_text_properties(element["level"].as<uint8_t>()),
            .callback = strlen(callbackContent) > 0 ? strdup(callbackContent) : strdup(""),
            .padding_x = static_cast<int16_t>(constrain(element["padding_x"] | 10, 0, 100)),
            .padding_y = static_cast<int16_t>(constrain(element["padding_y"] | 5, 0, 50)),
            .radius = static_cast<uint16_t>(constrain(element["radius"] | 0, 0, 35)),
            .filled = element["filled"] | false,
            .active = true,
            .changed = true,
            .updated = true,
            .bounds = {0}};

        // Find existing element
        ButtonElement_t *existingElement = findButtonElement(newButtonElement.id);
        if (existingElement != NULL)
        {
            // Compare newButtonElement with existingElement
            if (isButtonElementEqual(*existingElement, newButtonElement))
            {
                // Elements are the same
                existingElement->updated = true;
                existingElement->changed = false;
                // Free the newButtonElement.text since we don't need it
                free(newButtonElement.text);
                free(newButtonElement.callback);
            }
            else
            {
                // Elements are different
                // Clear old area
                clearButtonElementArea(existingElement, framebuffer);

                // Free old text
                free(existingElement->text);
                free(existingElement->callback);

                // Update existing element with new data
                *existingElement = newButtonElement;
                existingElement->changed = true;
                existingElement->updated = true;
            }
        }
        else
        {
            // New element
            storeButtonElement(newButtonElement);
        }
    }

    // Remove elements that are no longer present
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (buttonElements[i].active && !buttonElements[i].updated)
        {
            // Element was not updated, so it's removed
            // Clear its area
            clearButtonElementArea(&buttonElements[i], framebuffer);

            // Free text
            if (buttonElements[i].text)
            {
                free(buttonElements[i].text);
                free(buttonElements[i].callback);
            }
            // Mark as inactive
            buttonElements[i].active = false;
            buttonElementCount--;
        }
    }

    // Re-draw all active elements to buffer
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (buttonElements[i].active)
        {
            // If element changed, update display area
            if (buttonElements[i].changed)
            {
                // Update display area
                epd_push_pixels(buttonElements[i].bounds, 50, 1);
            }

            // Re-draw button to buffer
            drawButton(&buttonElements[i], framebuffer);
        }
    }
}

// Add this function to draw_button.h
bool isPointInRect(int16_t x, int16_t y, const Rect_t &rect)
{
    return (x >= rect.x && x < (rect.x + rect.width) &&
            y >= rect.y && y < (rect.y + rect.height));
}

bool customButtonCallback(ButtonElement_t *element, uint8_t *framebuffer)
{
    // custom functions are "refresh" in the callback
    if (element->callback && strlen(element->callback) > 0)
    {
        if (strcmp(element->callback, "toggle-dark") == 0)
        {
            if (current_display.background_color == 15)
            {
                set_black_display_mode();
            }
            else
            {
                set_white_display_mode();
            }
            epd_clear();
            set_background(framebuffer);
            return true;
        }
        if (strcmp(element->callback, "refresh") == 0)
        {
            return true;
        }
    }
    return false;
}

void executeButtonCallback(ButtonElement_t *element)
{
    if (element->callback && strlen(element->callback) > 0)
    {
        Serial.printf("executing callback: %s\n", element->callback);
        makeQuickPost(element->callback);
    }
}

// Add this function to handle button touches
bool handleButtonTouches(int16_t touch_x, int16_t touch_y, uint8_t *framebuffer)
{
    for (int i = 0; i < MAX_BUTTON_ELEMENTS; i++)
    {
        if (buttonElements[i].active)
        {
            if (isPointInRect(touch_x, touch_y, buttonElements[i].bounds))
            {
                epd_poweron();
                Serial.printf("button %d touched\n", buttonElements[i].id);
                // clearButtonElementArea(&buttonElements[i], framebuffer);
                drawButton(&buttonElements[i], framebuffer, true);
                epd_draw_grayscale_image(epd_full_screen(), framebuffer);
                bool customButtonCallbackResult = customButtonCallback(&buttonElements[i], framebuffer);
                if (customButtonCallbackResult)
                {
                    return true;
                }
                executeButtonCallback(&buttonElements[i]);
                drawButton(&buttonElements[i], framebuffer);
                epd_draw_grayscale_image(epd_full_screen(), framebuffer);
                epd_poweroff();
            }
        }
    }
    return true;
}

#endif // DRAW_BUTTON_H
