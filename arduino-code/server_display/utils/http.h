#ifndef UTILS_HTTP_H
#define UTILS_HTTP_H

#include <WiFi.h>
#include <HTTPClient.h>
#include "../config.h"

typedef struct {
    bool success;
    String response;
    int httpCode;
} ApiResponse_t;

#pragma region HTTP Methods
ApiResponse_t makeGetRequest(const char *url) {
    ApiResponse_t response = {false, "", -1};

    if (WiFi.status() != WL_CONNECTED) {
        LOG_E("WiFi not connected!");
        return response;
    }

    HTTPClient http;
    LOG_I("Making GET request to: %s", url);

    http.begin(url);
    response.httpCode = http.GET();

    if (response.httpCode > 0) {
        LOG_D("HTTP GET Response code: %d", response.httpCode);
        if (response.httpCode == HTTP_CODE_OK) {
            response.success = true;
            response.response = http.getString();
        }
    } else {
        LOG_E("Error sending GET: %d", response.httpCode);
    }

    http.end();
    return response;
}

ApiResponse_t makePostRequest(const char *url, const char *payload = "") {
    ApiResponse_t response = {false, "", -1};

    if (WiFi.status() != WL_CONNECTED) {
        LOG_E("WiFi not connected!");
        return response;
    }

    HTTPClient http;
    LOG_I("Making POST request to: %s", url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    response.httpCode = http.POST(payload);

    if (response.httpCode > 0) {
        LOG_D("HTTP POST Response code: %d", response.httpCode);
        if (response.httpCode == HTTP_CODE_OK) {
            response.success = true;
            response.response = http.getString();
        }
    } else {
        LOG_E("Error sending POST: %d", response.httpCode);
    }

    http.end();
    return response;
}

bool makeQuickPost(const char *callback) {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_E("WiFi not connected!");
        return false;
    }

    String url = String(BASE_URL) + String(callback);
    HTTPClient http;
    http.begin(url);
    int httpCode = http.POST("");
    http.end();

    return httpCode == HTTP_CODE_OK;
}

#pragma endregion

#pragma region Image Methods

bool fetchImageFromServer(String url, uint8_t *img_buffer, uint32_t &received_width, uint32_t &received_height, const uint32_t width, const uint32_t height) {
    HTTPClient http;

    LOG_D("URL: %s", url.c_str());
    http.setTimeout(10000); // Set timeout to 10 seconds
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        LOG_E("HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    // Get dimensions from header
    if (http.getStream().readBytes((char *)&received_width, 4) != 4 ||
        http.getStream().readBytes((char *)&received_height, 4) != 4) {
        LOG_E("Failed to read image dimensions");
        http.end();
        return false;
    }

    // Calculate size and read image data
    size_t bytes_per_row = width / 2;
    size_t image_data_size = bytes_per_row * height;

    if (http.getStream().readBytes((char *)img_buffer, image_data_size) != image_data_size) {
        LOG_E("Failed to read complete image data");
        http.end();
        return false;
    }

    http.end();
    LOG_D("Fetched image from server");
    return true;
}
#pragma endregion

#endif // UTILS_HTTP_H