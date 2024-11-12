import io
import struct
from PIL import Image
from flask import Blueprint, request, jsonify, send_file
from utils.convert import convert_image_to_epd_data, convert_image_bytes_to_epd_data
import datetime
from utils.spotify import get_track_info, get_album_art, get_track_liked_status
import os
import json
import requests

EPD_WIDTH = 960
EPD_HEIGHT = 540
LAYOUTS_DIR = 'layouts'

# Create blueprint
pages = Blueprint('pages', __name__) 

# Configuration
URL = 'http://localhost:5173/'

def process_album_art(width, height):
    """Handle fetching and processing of Spotify album art."""
    album_art = get_album_art()
    if not album_art or 'url' not in album_art:
        raise FileNotFoundError("No album art available")
    
    response = requests.get(album_art['url'])
    if response.status_code != 200:
        raise Exception("Failed to download album art")
    
    image_data = io.BytesIO(response.content)
    with Image.open(image_data) as image:
        return convert_image_to_epd_data(image, width, height, process_image=True)

def process_icon(image_name, filled, width, height):
    """Handle processing of local icon files."""
    icon_type = 'solid_png' if filled else 'outline_png'
    file_path = os.path.normpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), 
        f'../icons/{icon_type}', 
        image_name
    ))

    with Image.open(file_path) as image:
        return convert_image_to_epd_data(image, width, height, process_image=False)

def create_binary_response(width, height, raw_data):
    """Create binary response stream with image data."""
    stream = io.BytesIO()
    stream.write(struct.pack('<II', width, height))
    stream.write(raw_data)
    stream.seek(0)
    return stream

@pages.route('/icon/<name>', methods=['GET'])
def get_icon(name):
    try:
        # Get dimensions from query parameters or use defaults
        width = int(request.args.get('width', 100))
        height = int(request.args.get('height', 100))
        filled = request.args.get('filled', True)
        
        # Add .png if not present
        if not name.endswith('.png'):
            name += '.png'

        # Process icon
        width, height, raw_data = process_icon(name, filled, width, height)

        # Create and return binary response
        stream = create_binary_response(width, height, raw_data)
        return send_file(
            stream,
            mimetype='application/octet-stream',
            download_name='image.bin',
            as_attachment=True
        )

    except FileNotFoundError as e:
        print(f"File not found error: {str(e)}")
        return jsonify({'error': f'Icon {name} not found'}), 404
    except Exception as e:
        print(f"Error in icon route: {str(e)}")
        return jsonify({'error': str(e)}), 500

@pages.route('/album-art/', methods=['GET'])
def get_current_album_art():
    try:
        # Get dimensions from query parameters or use defaults
        width = int(request.args.get('width', 100))
        height = int(request.args.get('height', 100))

        # Process album art
        width, height, raw_data = process_album_art(width, height)

        # Create and return binary response
        stream = create_binary_response(width, height, raw_data)
        return send_file(
            stream,
            mimetype='application/octet-stream',
            download_name='image.bin',
            as_attachment=True
        )

    except FileNotFoundError as e:
        print(f"File not found error: {str(e)}")
        return jsonify({'error': 'Current album art not found'}), 404
    except Exception as e:
        print(f"Error in album-art route: {str(e)}")
        return jsonify({'error': str(e)}), 500

@pages.route('/home', methods=['GET'])
def home():
    try:
        track_info = get_track_info()
        time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        # get latest layout file from layouts dir
        latest_layout_path = os.path.join(LAYOUTS_DIR, 'latest.json')
        if os.path.exists(latest_layout_path):
            with open(latest_layout_path, 'r') as file:
                layout_data = json.load(file)
        else:
            return jsonify({'error': 'No layout found'}), 404
        
        # if the layout_data elements "text" equals "track_info" or "time" replace it with the variables
        for element in layout_data['elements']:
            if element['text'] == 'track_info':
                if track_info['artist'] is not None:
                    element['text'] = f"Now playing: {track_info['title']} by {track_info['artist']}"
                else:
                    element['text'] = "Nothing playing"
            elif element['text'] == 'time':
                element['text'] = time
    
        
        response = {
            "clear": False,
            "elements": layout_data['elements'],
        }
        print(response)
        return jsonify(response)
    
    except Exception as e:
        print(f"Error in home route: {str(e)}")
        return jsonify({'error': str(e)}), 500