// layout.h
#ifndef LAYOUT_H
#define LAYOUT_H

#include "../config.h"
#include "http.h"
#include <ArduinoJson.h>

#pragma region Page Definitions
typedef enum {
    PAGE_HOME,
    PAGE_MUSIC,
    PAGE_COUNT
} ApiPage_t;

static const char* PAGE_URLS[] = {
    "/home",
    "/music",
};

#pragma endregion

#pragma region Layout Functions
/**
 * @brief Make a GET request to a specific page
 * @param page The page to request (defaults to PAGE_HOME)
 * @return ApiResponse_t containing success status and response
 */
ApiResponse_t getPage(ApiPage_t page = PAGE_HOME) {
    String url = String(BASE_URL) + String("/pages") + String(PAGE_URLS[page]);
    return makeGetRequest(url.c_str());
}

/**
 * @brief Parse JSON response into a JsonDocument
 * @param jsonResponse The JSON string to parse
 * @return DynamicJsonDocument containing parsed JSON
 */
DynamicJsonDocument parseJsonResponse(const String& jsonResponse) {
    DynamicJsonDocument doc(MAX_JSON_SIZE);
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error) {
        LOG_E("JSON parsing failed: %s", error.c_str());
        return DynamicJsonDocument(0);
    }

    return doc;
}
#pragma endregion

#endif // LAYOUT_H