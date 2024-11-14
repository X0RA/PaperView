#ifndef UTILS_EINK_H
#define UTILS_EINK_H

#include "../config.h"
#include "epd_driver.h"
#include <Arduino.h>
// Display properties structure
typedef struct
{
    uint8_t background_color; // 0 = black, 15 = white
    uint8_t foreground_color; // 0 = black, 15 = white
    FontProperties primary;
    FontProperties secondary;
    FontProperties tertiary;
    FontProperties quaternary;
} display_properties_t;

// Constant display properties for white background
const display_properties_t WHITE_DISPLAY = {
    .background_color = 15, // White background
    .foreground_color = 0,
    .primary = {// Black text
                .fg_color = 0,
                .bg_color = 15,
                .fallback_glyph = 0,
                .flags = 0},
    .secondary = {// Dark grey
                  .fg_color = 4,
                  .bg_color = 15,
                  .fallback_glyph = 0,
                  .flags = 0},
    .tertiary = {// Medium grey
                 .fg_color = 8,
                 .bg_color = 15,
                 .fallback_glyph = 0,
                 .flags = 0},
    .quaternary = {// Light grey
                   .fg_color = 12,
                   .bg_color = 15,
                   .fallback_glyph = 0,
                   .flags = 0}};

// Constant display properties for black background
const display_properties_t BLACK_DISPLAY = {
    .background_color = 0, // Black background
    .foreground_color = 15,
    .primary = {// White text
                .fg_color = 15,
                .bg_color = 0,
                .fallback_glyph = 0,
                .flags = 0},
    .secondary = {// Light grey
                  .fg_color = 12,
                  .bg_color = 0,
                  .fallback_glyph = 0,
                  .flags = 0},
    .tertiary = {// Medium grey
                 .fg_color = 8,
                 .bg_color = 0,
                 .fallback_glyph = 0,
                 .flags = 0},
    .quaternary = {// Dark grey
                   .fg_color = 4,
                   .bg_color = 0,
                   .fallback_glyph = 0,
                   .flags = 0}};

// Current display properties - initialize to white display
display_properties_t current_display = WHITE_DISPLAY;

/**
 * @brief Clear a specific area of the display using current background color in framebuffer
 */
void clear_area(Rect_t area, uint8_t *framebuffer) {
    if (!framebuffer)
        return;

    LOG_D("Clearing area: %d, %d, %d, %d", area.x, area.y, area.width, area.height);

    // Convert background color to 4-bit value (0-15)
    uint8_t fill_value = current_display.background_color & 0x0F;

    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT)
                continue;

            int idx = (y * EPD_WIDTH + x) / 2;
            if (x % 2 == 0) {
                framebuffer[idx] = (framebuffer[idx] & 0x0F) | (fill_value << 4);
            } else {
                framebuffer[idx] = (framebuffer[idx] & 0xF0) | fill_value;
            }
        }
    }
}

/**
 * @brief Set the entire display background using current background color
 */
bool set_background(uint8_t *framebuffer) {
    if (!framebuffer) {
        LOG_E("Error: Framebuffer is null");
        return false;
    }

    Rect_t full_screen = epd_full_screen();
    clear_area(full_screen, framebuffer);
    return true;
}

/**
 * @brief Get FontProperties for text level
 */
FontProperties get_text_properties(uint8_t level) {
    switch (level) {
    case 1:
        return current_display.primary;
    case 2:
        return current_display.secondary;
    case 3:
        return current_display.tertiary;
    case 4:
        return current_display.quaternary;
    default:
        return current_display.primary;
    }
}

void set_black_display_mode() {
    current_display = BLACK_DISPLAY;
}

void set_white_display_mode() {
    current_display = WHITE_DISPLAY;
}

void set_custom_display_mode(display_properties_t new_display) {
    current_display = new_display;
}

/**
 * @brief Refresh the display by clearing and powering off (4 times)
 */
void deep_refresh() {
    LOG_D("Deep refreshing display");
    epd_poweron();
    epd_clear();
    epd_poweroff();
}

#endif // UTILS_EINK_H