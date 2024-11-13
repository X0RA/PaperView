#ifndef API_FUNCTIONS_H
#define API_FUNCTIONS_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "wifi_config.h"

// API Configuration
#define MAX_JSON_SIZE 8192

// Page definitions
typedef enum
{
    PAGE_HOME,
    PAGE_MUSIC,
    PAGE_COUNT
} ApiPage_t;

// Page URLs (must match order of ApiPage_t)
static const char *PAGE_URLS[] = {
    "/home",
    "/music",
};

typedef struct
{
    bool success;
    String response;
    int httpCode;
} ApiResponse_t;

/**
 * @brief Make a GET request to a specific page
 * @param page The page to request (defaults to PAGE_HOME)
 * @return ApiResponse_t containing success status and response
 */
ApiResponse_t getPage(ApiPage_t page = PAGE_HOME)
{
    ApiResponse_t response = {false, "", -1};

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return response;
    }

    HTTPClient http;
    String url = String(BASE_URL) + String("/pages") + String(PAGE_URLS[page]);

    Serial.println("Making GET request to: " + url);

    http.begin(url);
    response.httpCode = http.GET();

    if (response.httpCode > 0)
    {
        Serial.printf("HTTP GET Response code: %d\n", response.httpCode);
        if (response.httpCode == HTTP_CODE_OK)
        {
            response.success = true;
            response.response = http.getString();
        }
    }
    else
    {
        Serial.printf("Error sending GET: %d\n", response.httpCode);
    }

    http.end();
    return response;
}

/**
 * @brief Make a GET request to a specific URL
 * @param url The full URL to request
 * @return ApiResponse_t containing success status and response
 */
ApiResponse_t makeGetRequest(const char *url)
{
    ApiResponse_t response = {false, "", -1};

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return response;
    }

    HTTPClient http;
    Serial.println("Making GET request to: " + String(url));

    http.begin(url);
    response.httpCode = http.GET();

    if (response.httpCode > 0)
    {
        Serial.printf("HTTP GET Response code: %d\n", response.httpCode);
        if (response.httpCode == HTTP_CODE_OK)
        {
            response.success = true;
            response.response = http.getString();
        }
    }
    else
    {
        Serial.printf("Error sending GET: %d\n", response.httpCode);
    }

    http.end();
    return response;
}


/**
 * @brief Make a POST request to a specific endpoint
 * @param endpoint The endpoint to send the POST request to
 * @param payload The JSON payload to send (optional)
 * @return ApiResponse_t containing success status and response
 */
ApiResponse_t makePostRequest(const char *endpoint, const char *payload = "")
{
    ApiResponse_t response = {false, "", -1};

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return response;
    }

    HTTPClient http;
    String url = String(BASE_URL) + String(endpoint);

    Serial.println("Making POST request to: " + url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    response.httpCode = http.POST(payload);

    if (response.httpCode > 0)
    {
        Serial.printf("HTTP POST Response code: %d\n", response.httpCode);
        if (response.httpCode == HTTP_CODE_OK)
        {
            response.success = true;
            response.response = http.getString();
        }
    }
    else
    {
        Serial.printf("Error sending POST: %d\n", response.httpCode);
    }

    http.end();
    return response;
}

void makeQuickPost(const char *endpoint)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return;
    }

    HTTPClient http;
    String url = String(BASE_URL) + String(endpoint);

    http.begin(url);
    int httpCode = http.POST("");

    http.end();
}

/**
 * @brief Parse JSON response into a JsonDocument
 * @param jsonResponse The JSON string to parse
 * @return DynamicJsonDocument containing parsed JSON
 */
DynamicJsonDocument parseJsonResponse(const String &jsonResponse)
{
    DynamicJsonDocument doc(MAX_JSON_SIZE);
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error)
    {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return DynamicJsonDocument(0);
    }

    return doc;
}

#endif // API_FUNCTIONS_H