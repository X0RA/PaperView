#ifndef UTILS_EINK_H
#define UTILS_EINK_H

#include "../config.h"
#include "epd_driver.h"
#include <Arduino.h>
#include "types.h"

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
void clear_framebuffer_area(Rect_t area, uint8_t *framebuffer) {
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
 * @brief Push pixels to a specific area of the display with a default 2 cycle refresh
 */
void clear_area(Rect_t area, uint8_t *framebuffer, int32_t cycles = 2, int16_t bg_time = 50, int16_t fg_time = 50) {
    if (!framebuffer)
        return;

    // NOTE: Ya wanna end on the background color

    int32_t bg_color = current_display.background_color == 0 ? 0 : 1;
    int32_t fg_color = bg_color == 0 ? 1 : 0;

    for (int32_t c = 0; c < cycles; c++) {
        for (int32_t i = 0; i < 4; i++) {
            epd_push_pixels(area, fg_time, fg_color);
        }
        for (int32_t i = 0; i < 4; i++) {
            epd_push_pixels(area, bg_time, bg_color);
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
    clear_framebuffer_area(full_screen, framebuffer);
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

void refresh_display(RefreshType refresh_type, uint8_t *framebuffer) {
    if (refresh_type == NO_REFRESH || refresh_type == REFETCH_ELEMENTS)
        return;
    if (!framebuffer)
        return;

    Rect_t full_screen = epd_full_screen();
    switch (refresh_type) {
    case DISPLAY_REFRESH_COMPLETE:
        LOG_D("Display complete refresh");
        clear_area(full_screen, framebuffer, 4, 50, 50);
        break;
    case DISPLAY_REFRESH_PARTIAL:
        LOG_D("Display partial refresh");
        clear_area(full_screen, framebuffer, 2, 50, 50);
        break;
    case DISPLAY_REFRESH_FAST:
        LOG_D("Display fast refresh");
        if (!framebuffer)
            return;
        int32_t bg_color = current_display.background_color == 0 ? 0 : 1;
        epd_push_pixels(full_screen, 50, bg_color);
        break;
    }
}

void refresh_area(RefreshType refresh_type, uint8_t *framebuffer, Rect_t area) {
    if (refresh_type == NO_REFRESH || refresh_type == REFETCH_ELEMENTS)
        return;
    if (!framebuffer)
        return;

    switch (refresh_type) {
    case ELEMENT_REFRESH_COMPLETE:
        LOG_D("Element complete refresh");
        clear_area(area, framebuffer, 4, 50, 50);
        break;
    case ELEMENT_REFRESH_PARTIAL:
        LOG_D("Element partial refresh");
        clear_area(area, framebuffer, 2, 50, 50);
        break;
    case ELEMENT_REFRESH_FAST:
        LOG_D("Element fast refresh");
        if (!framebuffer)
            return;
        int32_t bg_color = current_display.background_color == 0 ? 0 : 1;
        epd_push_pixels(area, 50, bg_color);
        break;
    }
}

/**
 * @brief Refresh the display by clearing and powering off (4 times)
 */
void eink_full_refresh() {
    epd_poweron();
    epd_clear();
    epd_poweroff();
}

/**
 * @brief Draw the display by drawing the framebuffer to the epd
 */
void draw_framebuffer(uint8_t *framebuffer) {
    LOG_D("Drawing framebuffer");
    epd_poweron();
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    epd_poweroff();
}

#endif // UTILS_EINK_H