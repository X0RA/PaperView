#ifndef LIFECYCLE_MANAGER_H
#define LIFECYCLE_MANAGER_H

#include <Arduino.h>
#include "element_manager.h"
#include "../utils/api_functions.h"

/**
 * @brief Update the display with the latest data from the API
 * @param framebuffer The framebuffer to update
 * @param doClear Whether to clear the display before updating
 * @return True if the display was updated successfully, false otherwise
 */
bool update_display(uint8_t *framebuffer, ElementManager &elementManager, bool doClear)
{
    if (!framebuffer)
    {
        Serial.println("Framebuffer is null!");
        return false;
    }

    epd_poweron();

    // Get data from API
    ApiResponse_t getResponse = getPage(PAGE_HOME);
    if (!getResponse.success)
    {
        Serial.println("Failed to get API response");
        epd_poweroff();
        return false;
    }

    // Parse JSON response
    DynamicJsonDocument doc = parseJsonResponse(getResponse.response);
    if (doc.isNull())
    {
        Serial.println("Failed to parse JSON data");
        epd_poweroff();
        return false;
    }

    // Check if we should clear the display
    bool shouldClear = false;
    if (doc.containsKey("clear"))
    {
        shouldClear = doc["clear"].as<bool>();
        Serial.printf("Clear flag from server: %s\n", shouldClear ? "true" : "false");
    }

    if (shouldClear || doClear)
    {
        Serial.println("Clearing display");
        epd_clear();
        // Also clear framebuffer
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    }

    set_background(framebuffer);

    // Get the elements array
    JsonArray elements = doc["elements"];
    if (elements.isNull())
    {
        Serial.println("No elements found in JSON");
        epd_poweroff();
        return false;
    }

    elementManager.processElements(elements, framebuffer);

    epd_draw_grayscale_image(epd_full_screen(), framebuffer);

    epd_poweroff();
    return true;
}

#endif // LIFECYCLE_MANAGER_H   