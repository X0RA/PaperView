import io
import struct
from PIL import Image
from flask import Blueprint, request, jsonify, send_file
from utils.convert import convert_image_to_epd_data, convert_image_bytes_to_epd_data
import datetime
from utils.spotify import get_track_info, get_album_art, get_track_liked_status


EPD_WIDTH = 960
EPD_HEIGHT = 540

# Create blueprint
pages = Blueprint('pages', __name__) 

# Configuration
URL = 'http://localhost:5173/'


image_width = 100
image_height  = 100


@pages.route('/image', methods=['GET'])
def display():
    try:
        # Get dimensions from query parameters or use defaults
        width = int(request.args.get('width', image_width))
        height = int(request.args.get('height', image_height))

        # Open and process image
        with Image.open("testtest.png") as image:
            width, height, raw_data = convert_image_to_epd_data(image, width, height)


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
            mimetype='application/octet-stream',
            download_name='image.bin',
            as_attachment=True
        )
    except FileNotFoundError:
        return jsonify({'error': 'Image file not found'}), 404
    except Exception as e:
        print(f"Error in display route: {str(e)}")
        return jsonify({'error': str(e)}), 500
        
    
    

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
        
        # Constants for layout
        TRACK_INFO_Y = 100  # Y position for track info
        CONTROLS_Y = TRACK_INFO_Y + 50  # Y position for controls (50px below track info)
        CONTROLS_START_X = 5  # Starting X position for controls
        CONTROLS_SPACING = 135  # Spacing between controls
        
        response = {
            "clear": False,
            "elements": [
                # Clock at top
                create_text_element(
                    1, 
                    f"{datetime.datetime.now().strftime('%A %H:%M:%S')}", 
                    EPD_WIDTH / 2, 
                    EPD_HEIGHT - 200, 
                    1, 
                    "bl"
                ),
                
                # Track info
                create_text_element(
                    2, 
                    f"{track_info['artist']}, {track_info['title']}", 
                    CONTROLS_START_X, 
                    TRACK_INFO_Y, 
                    1, 
                    "bl"
                ),
                
                # Playback controls - note the changed anchor points
                create_button_element(
                    3, 
                    "Previous", 
                    CONTROLS_START_X, 
                    CONTROLS_Y, 
                    "/spotify/playback/previous", 
                    anchor="tr",  # Right edge of button at x position
                    padding_x=20
                ),
                create_button_element(
                    4, 
                    "Play", 
                    CONTROLS_START_X + CONTROLS_SPACING + 100, 
                    CONTROLS_Y, 
                    "/spotify/playback/toggle", 
                    anchor="tr",
                    padding_x=20
                ),
                create_button_element(
                    5, 
                    "Next", 
                    CONTROLS_START_X + (CONTROLS_SPACING * 2)  + 100, 
                    CONTROLS_Y, 
                    "/spotify/playback/next", 
                    anchor="tr",
                    padding_x=20
                ),
                
                # Utility buttons at bottom
                create_button_element(
                    7, 
                    "Refresh", 
                    EPD_WIDTH, 
                    EPD_HEIGHT, 
                    "refresh", 
                    anchor="bl"
                ),
                create_button_element(
                    8, 
                    "Toggle", 
                    0, 
                    EPD_HEIGHT, 
                    "toggle-dark", 
                    anchor="br"
                ),
            ]
        }
        print(response)
        return jsonify(response)
    
    except Exception as e:
        print(f"Error in home route: {str(e)}")
        return jsonify({'error': str(e)}), 500