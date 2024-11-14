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

void makeQuickPost(const char *url) {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_E("WiFi not connected!");
        return;
    }

    HTTPClient http;
    http.begin(url);
    http.POST("");
    http.end();
}
#pragma endregion

#endif // UTILS_HTTP_H