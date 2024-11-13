from flask import Blueprint, request, jsonify, send_file
import datetime
from utils.spotify import (
    get_spotify_client,
    get_current_track_info,
    toggle_playback_state,
    skip_to_next,
    skip_to_previous
)
import os
import json

# Create blueprint
pages = Blueprint('pages', __name__) 

LAYOUTS_DIR = 'layouts'


def get_layout(layout_name):
    #find the layout in the layouts dir if it exists, return the json data
    layout_path = os.path.join(LAYOUTS_DIR, f"{layout_name}.json")
    if os.path.exists(layout_path):
        with open(layout_path, 'r') as file:
            return json.load(file)
    else:
        return None


def parse_layout_data(layout_data):
    sp = get_spotify_client()
    track_info = get_current_track_info(sp)
    time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    for element in layout_data['elements']:
        if element['text'] == 'track_info':
            if track_info is None:
                element['text'] = "No track currently playing"
            else:
                element['text'] = f"Now playing: {track_info['track']} by {track_info['artist']}"
        elif element['text'] == 'time':
            element['text'] = time
        if element['path'] == 'album-art':
            if track_info is None:
                element['name'] = "None"
            else:
                name = f"{track_info['artist']}_{track_info['track']}"
                # make sure the name is 16 characters or less and standard utf-8
                name = name[:16]
                # Replace non-ascii characters and special characters with underscores
                name = ''.join(c if c.isalnum() or c == '_' else '_' for c in name)
                # Ensure it's valid UTF-8
                name = name.encode('ascii', 'ignore').decode('ascii')
                element['name'] = name
    return layout_data

@pages.route('/<layout_name>', methods=['GET'])
def get_layout_data(layout_name):
    print(f"Getting layout data for {layout_name}")
    layout_data = get_layout(layout_name)
    parsed_layout_data = parse_layout_data(layout_data) 
    return jsonify(parsed_layout_data)