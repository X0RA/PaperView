#ifndef TEXT_ELEMENT_H
#define TEXT_ELEMENT_H

#include "element.h"

class TextElement : public DrawElement {

public:
    TextElement() : DrawElement() {
        type = ElementType::TEXT;
    }

    void draw(uint8_t *framebuffer) override {
        if (!text)
            return;

        int32_t x1, y1, w, h;
        int32_t temp_x = x;
        int32_t temp_y = y;

        get_text_bounds((GFXfont *)&FiraSans, text,
                        &temp_x, &temp_y,
                        &x1, &y1, &w, &h, NULL);

        int32_t cursor_x = x;
        int32_t cursor_y = y;

        switch (anchor) {
        case Anchor::TOP_LEFT:
            cursor_y = y + h;
            break;
        case Anchor::TOP_RIGHT:
            cursor_x = x - w;
            cursor_y = y + h;
            break;
        case Anchor::TOP_MIDDLE:
            cursor_x = x - (w / 2);
            cursor_y = y + h;
            break;
        case Anchor::MIDDLE:
            cursor_x = x - (w / 2);
            cursor_y = y + (h / 2);
            break;
        case Anchor::BOTTOM_LEFT:
            break;
        case Anchor::BOTTOM_MIDDLE:
            cursor_x = x - (w / 2);
            break;
        case Anchor::BOTTOM_RIGHT:
            cursor_x = x - w;
            break;
        }

        // Calculate actual text boundaries after anchor adjustments
        int32_t text_left = cursor_x;
        int32_t text_top = cursor_y - h;
        int32_t text_right = cursor_x + w;
        int32_t text_bottom = cursor_y;

        // Store bounds with proper clipping to screen edges
        bounds = {
            .x = max(0, min(EPD_WIDTH - 1, text_left)),
            .y = max(0, min(EPD_HEIGHT - 1, text_top)),
            .width = min(w, EPD_WIDTH - bounds.x),
            .height = min(h, EPD_HEIGHT - bounds.y)};

        write_mode((GFXfont *)&FiraSans, text,
                   &cursor_x, &cursor_y,
                   framebuffer,
                   BLACK_ON_WHITE,
                   &font_props);

        active = true;
    }

    bool updateFromJson(JsonObject &element) override {
        if (strcmp(element["type"] | "", "text") != 0)
            return false;

        const char *textContent = element["text"] | "";
        if (strlen(textContent) == 0)
            return false;

        id = element["id"].as<uint16_t>();
        text = strdup(textContent);
        x = element["x"].as<int16_t>();
        y = element["y"].as<int16_t>();
        anchor = getAnchorFromString(element["anchor"] | "bl");
        font_props = get_text_properties(element["level"].as<uint8_t>());
        active = true;
        changed = true;
        updated = true;

        return true;
    }

    void drawTouched(uint8_t *framebuffer) override {
        // Text elements don't have a touched state, so just draw normally
        draw(framebuffer);
    }

    void updateElement() override {
        // TODO: Implement
        return;
    }
};

#endif // TEXT_ELEMENT_H