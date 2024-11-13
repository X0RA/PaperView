import io
import struct
from PIL import Image
from flask import Blueprint, request, jsonify, send_file
from utils.convert import convert_image_to_epd_data
import os
import requests
from utils.spotify import (
    get_spotify_client,
    get_current_track_info,
    toggle_playback_state,
    skip_to_next,
    skip_to_previous
)

# Create blueprint
images = Blueprint('images', __name__) 

def process_album_art(width, height):
    """Handle fetching and processing of Spotify album art."""
    sp = get_spotify_client()
    track_info = get_current_track_info(sp)
    
    if not track_info or not track_info.get('album_art'):
        raise FileNotFoundError("No album art available")
    
    response = requests.get(track_info['album_art'])
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

@images.route('/icon/<name>', methods=['GET'])
def get_icon(name):
    try:
        width = int(request.args.get('width', 100))
        height = int(request.args.get('height', 100))
        filled = request.args.get('filled', True)
        
        if not name.endswith('.png'):
            name += '.png'

        width, height, raw_data = process_icon(name, filled, width, height)

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

@images.route('/album-art/', methods=['GET'])
def get_current_album_art():
    try:
        width = int(request.args.get('width', 100))
        height = int(request.args.get('height', 100))

        width, height, raw_data = process_album_art(width, height)

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

