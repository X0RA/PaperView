import io
import struct
from flask import Blueprint, request, jsonify, send_file
from utils.convert_rotate import convert_image_to_epd_data, convert_image_bytes_to_epd_data
from utils.get_web import get_screenshot_data, get_screenshot_bytes
import datetime
from utils.spotify import get_track_info, get_album_art, get_track_liked_status


EPD_WIDTH = 960
EPD_HEIGHT = 540

# Create blueprint
pages = Blueprint('pages', __name__) 

# Configuration
URL = 'http://localhost:5173/'

@pages.route('/display', methods=['GET'])
def display():
    try:
        # Get screenshot as bytes
        screenshot_bytes = get_screenshot_bytes(URL)
        
        # Convert the screenshot to EPD format
        width, height, raw_data = convert_image_bytes_to_epd_data(screenshot_bytes)
        
        # Create binary stream
        stream = io.BytesIO()
        
        # Pack width and height as 32-bit unsigned integers
        stream.write(struct.pack('<II', width, height))
        
        # Write the processed image data
        stream.write(raw_data)
        
        # Reset stream position
        stream.seek(0)
        
        return send_file(
            stream,
            mimetype='application/octet-stream'
        )
    except Exception as e:
            print(f"Error in display route: {str(e)}") 
            return jsonify({'error': str(e)}), 500
        
    
#  // Text element structure
# typedef struct
# {
#     uint16_t id;
#     char *text;
#     int16_t x;
#     int16_t y;
#     TextAnchor_t anchor;
#     FontProperties color;
#     Rect_t bounds;
#     bool active;
# } TextElement_t;   

# // Constant display properties for white background
# const display_properties_t WHITE_DISPLAY = {
#     .background_color = 15, // White background
#     .primary = {            // Black text
#                 .fg_color = 0,
#                 .bg_color = 15,
#                 .fallback_glyph = 0,
#                 .flags = 0},
#     .secondary = {// Dark grey
#                   .fg_color = 4,
#                   .bg_color = 15,
#                   .fallback_glyph = 0,
#                   .flags = 0},
#     .tertiary = {// Medium grey
#                  .fg_color = 8,
#                  .bg_color = 15,
#                  .fallback_glyph = 0,
#                  .flags = 0},
#     .quaternary = {// Light grey
#                    .fg_color = 12,
#                    .bg_color = 15,
#                    .fallback_glyph = 0,
#                    .flags = 0}};

# FontProperties get_text_properties(uint8_t level)
# {
#     switch (level)
#     {
#     case 1:
#         return current_display.primary;
#     case 2:
#         return current_display.secondary;
#     case 3:
#         return current_display.tertiary;
#     case 4:
#         return current_display.quaternary;
#     default:
#         return current_display.primary;
#     }
# }

def create_button_element(id: int, text: str, x: int, y: int, callback: str, filled: bool = True, anchor: str = "tl", radius: int = 20, padding_x: int = 10, padding_y: int = 10, level: int = 1) -> dict:
    """
    Creates a button element dictionary with the specified parameters.
    
    Args:
        id (int): Unique identifier for the button element
        text (str): The text content to display on the button
        x (int): X-coordinate position
        y (int): Y-coordinate position
        callback (str): The callback URL or action for the button
        filled (bool, optional): Whether the button should be filled. Defaults to True
        anchor (str, optional): Button anchor position. Defaults to "tl" (top-left)
        radius (int, optional): Corner radius of the button. Defaults to 20
        padding_x (int, optional): Horizontal padding. Defaults to 10
        padding_y (int, optional): Vertical padding. Defaults to 10
        level (int, optional): Button property level (1-4). Defaults to 1
        
    Returns:
        dict: A dictionary containing the button element properties
    """
    return {
        "id": id,
        "type": "button",
        "text": text,
        "x": x,
        "y": y,
        "anchor": anchor,
        "filled": filled,
        "radius": radius,
        "padding_x": padding_x,
        "padding_y": padding_y,
        "level": level,
        "callback": callback
    }


ids = []

def create_text_element(id: int, text: str, x: int, y: int, level: int = 0, anchor: str = "bl") -> dict:
    """
    Creates a text element dictionary with the specified parameters.
    
    Args:
        id (int): Unique identifier for the text element
        text (str): The text content to display
        x (int): X-coordinate position
        y (int): Y-coordinate position
        anchor (str, optional): Text anchor position. Defaults to "bl" (bottom-left)
        level (int, optional): Text property level (0-4). Defaults to 0
        
    Returns:
        dict: A dictionary containing the text element properties
    """
    # get id from 2 onwards if id is not provided
    return {
        "id": id,
        "type": "text",
        "text": text,
        "x": x,
        "y": y,
        "anchor": anchor,
        "level": level
    }
        
        
@pages.route('/home', methods=['GET'])
def home():
    try:
        track_info = get_track_info()
        response = {
            "clear": False,
            "elements": [
                create_text_element(1, f"{datetime.datetime.now().strftime('%A %H:%M')}", EPD_WIDTH / 2, 12, 1, "m"),
                create_text_element(2, f"{track_info['artist']}, {track_info['title']}",5, 100, 1, "bl"),
                create_button_element(1, "Next", 5, 150, "/spotify/playback/previous"),
                create_button_element(2, "Play", 140, 150, "/spotify/playback/toggle"),
                create_button_element(3, "Next", 260, 150, "/spotify/playback/next"),
                create_button_element(3, "Nothing", 360, 350, ""),
                # 
                create_button_element(5, "Refresh", EPD_WIDTH, EPD_HEIGHT, "refresh", anchor="br"),
                create_button_element(6, "Toggle", 400, 200, "toggle-dark")

                   
            ]
        }
        print(response)
        return jsonify(response)
    
    except Exception as e:
        print(f"Error in home route: {str(e)}")
        return jsonify({'error': str(e)}), 500