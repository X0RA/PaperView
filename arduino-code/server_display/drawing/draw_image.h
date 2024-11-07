
// Helper function to check if a point is within tolerance of target coordinates
bool isWithinArea(int16_t x, int16_t y, int16_t targetX, int16_t targetY, int16_t tolerance) {
    return (abs(x - targetX) <= tolerance && abs(y - targetY) <= tolerance);
}


// Define the touch areas with some tolerance
struct TouchArea {
    int16_t x;
    int16_t y;
    int16_t tolerance;
    const char* endpoint;
};

// Define our touch sensitive areas
const TouchArea touchAreas[] = {
    {348, 258, 40, "/spotify/playback/previous"},  // rewind
    {342, 186, 40, "/spotify/playback/toggle"},    // play
    {333, 63, 40, "/spotify/playback/next"}        // next
};

bool fetchAndDisplayData(bool clearScreen) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return false;
    }
    HTTPClient http;
    bool success = false;
    
    Serial.println("Fetching image data...");
    http.begin("http://192.168.60.75:5000/display");
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        // Get the binary data
        uint32_t width, height;
        uint8_t* data = (uint8_t*)http.getStream().readBytes((char*)&width, 4);
        data = (uint8_t*)http.getStream().readBytes((char*)&height, 4);
        
        Serial.printf("Image dimensions: %dx%d\n", width, height);
        
        // Read the image data directly into the framebuffer
        size_t len = http.getSize() - 8;  // Subtract 8 bytes for width and height
        if (len <= EPD_WIDTH * EPD_HEIGHT / 2) {
            http.getStream().readBytes((char*)framebuffer, len);
            
            // Display the image
            epd_poweron();


            if(clearScreen){
                Serial.println("Clearing screen...");
                epd_clear();
            }
            
            Rect_t area = {
                .x = 0,
                .y = 0,
                .width = width,
                .height = height
            };
            
            epd_draw_grayscale_image(area, framebuffer);
            epd_poweroff();
            
            success = true;
            Serial.println("Image displayed successfully");
        } else {
            Serial.println("Image data too large for framebuffer");
        }
    } else {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
    return success;
}
