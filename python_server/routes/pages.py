import io
import struct
from PIL import Image
from flask import Blueprint, request, jsonify, send_file
from utils.convert import convert_image_to_epd_data, convert_image_bytes_to_epd_data
import datetime
from utils.spotify import get_track_info, get_album_art, get_track_liked_status
import os
import json

EPD_WIDTH = 960
EPD_HEIGHT = 540
LAYOUTS_DIR = 'layouts'

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
        with Image.open("icons/outline_png/adjustments-vertical.png") as image:
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