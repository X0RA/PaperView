#ifndef BUTTON_ELEMENT_H
#define BUTTON_ELEMENT_H

#include "DrawElement.h"
#include "../utils/api_functions.h"
#include "draw_display.h"

class ButtonElement : public DrawElement
{
private:
    char *callback;
    int16_t padding_x;
    int16_t padding_y;
    uint16_t radius;
    bool filled;
    bool touched;

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

    void draw_filled_rounded_rect(int16_t x, int16_t y, int16_t w, int16_t h,
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

public:
    ButtonElement() : DrawElement(), callback(nullptr),
                      padding_x(0), padding_y(0), radius(0),
                      filled(false), touched(false) {}

    ~ButtonElement()
    {
        if (callback)
        {
            free(callback);
        }
    }

    void draw(uint8_t *framebuffer) override
    {
        if (!text)
            return;

        // First get text dimensions
        int32_t x1, y1, w, h;
        int32_t temp_x = x;
        int32_t temp_y = y;
        get_text_bounds((GFXfont *)&FiraSans, text,
                        &temp_x, &temp_y,
                        &x1, &y1, &w, &h, NULL);

        // Calculate total button dimensions
        int32_t buttonWidth = w + (padding_x * 2);
        int32_t buttonHeight = h + (padding_y * 2);

        // Calculate button position based on anchor
        // Note: These calculations are now relative to the button's total size
        int32_t button_x = x;
        int32_t button_y = y;

        switch (anchor)
        {
        case Anchor::TOP_RIGHT:
            button_x = x; // Right edge aligns with x
            button_y = y; // Top edge aligns with y
            break;
        case Anchor::TOP_MIDDLE:
            button_x = x - (buttonWidth / 2); // Center horizontally on x
            button_y = y;                     // Top edge aligns with y
            break;
        case Anchor::TOP_LEFT:
            button_x = x - buttonWidth; // Left edge aligns with x
            button_y = y;               // Top edge aligns with y
            break;
        case Anchor::BOTTOM_LEFT:
            button_x = x - buttonWidth;  // Left edge aligns with x
            button_y = y - buttonHeight; // Bottom edge aligns with y
            break;
        case Anchor::BOTTOM_MIDDLE:
            button_x = x - (buttonWidth / 2); // Center horizontally on x
            button_y = y - buttonHeight;      // Bottom edge aligns with y
            break;
        case Anchor::BOTTOM_RIGHT:
            button_x = x;                // Right edge aligns with x
            button_y = y - buttonHeight; // Bottom edge aligns with y
            break;
        case Anchor::MIDDLE:
            button_x = x - (buttonWidth / 2);  // Center horizontally on x
            button_y = y - (buttonHeight / 2); // Center vertically on y
            break;
        }

        // Store the bounds for future clearing
        bounds = {
            .x = button_x,
            .y = button_y,
            .width = buttonWidth,
            .height = buttonHeight};

        // Determine background color based on touch state
        uint8_t background_color;
        if (touched)
        {
            background_color = props.bg_color == 0 ? 15 : 255;
        }
        else
        {
            background_color = props.bg_color == 15 ? 0 : 255;
        }

        // Draw the button
        if (filled)
        {
            draw_filled_rounded_rect(button_x, button_y, buttonWidth, buttonHeight,
                                     radius, background_color, framebuffer);
        }
        else
        {
            draw_rounded_rect(button_x, button_y, buttonWidth, buttonHeight,
                              radius, background_color, framebuffer);
        }

        // Calculate text position inside the button
        int32_t text_x = button_x + padding_x;
        int32_t text_y = button_y + padding_y + h;

        // Set up text properties
        FontProperties textProps = props;
        if (filled)
        {
            textProps.bg_color = props.fg_color;
            textProps.fg_color = props.bg_color;
        }

        // Draw the text
        write_mode((GFXfont *)&FiraSans, text,
                   &text_x, &text_y,
                   framebuffer,
                   BLACK_ON_WHITE,
                   &textProps);

        active = true;
    }

    bool updateFromJson(JsonObject &element) override
    {
        if (strcmp(element["type"] | "", "button") != 0)
            return false;

        const char *textContent = element["text"] | "";
        const char *callbackContent = element["callback"] | "";
        if (strlen(textContent) == 0)
            return false;

        id = element["id"].as<uint16_t>();
        text = strdup(textContent);
        callback = strdup(callbackContent);
        x = element["x"].as<int16_t>();
        y = element["y"].as<int16_t>();
        anchor = getAnchorFromString(element["anchor"] | "bl");
        props = get_text_properties(element["level"].as<uint8_t>());
        padding_x = static_cast<int16_t>(constrain(element["padding_x"] | 10, 0, 100));
        padding_y = static_cast<int16_t>(constrain(element["padding_y"] | 5, 0, 50));
        radius = static_cast<uint16_t>(constrain(element["radius"] | 0, 0, 35));
        filled = element["filled"] | false;
        active = true;
        changed = true;
        updated = true;

        return true;
    }

    bool executeCallback(uint8_t *framebuffer)
    {
        if (callback && strlen(callback) > 0)
        {
            if (strcmp(callback, "toggle-dark") == 0)
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
            if (strcmp(callback, "refresh") == 0)
            {
                return true;
            }
            makeQuickPost(callback);
        }
        return false;
    }

    void setTouched(bool touch)
    {
        touched = touch;
    }

    bool isPointInside(int16_t px, int16_t py) const
    {
        return (px >= bounds.x && px < (bounds.x + bounds.width) &&
                py >= bounds.y && py < (bounds.y + bounds.height));
    }
};

#endif // BUTTON_ELEMENT_H