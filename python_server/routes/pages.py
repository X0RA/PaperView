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
    # {
    #   "id": 0,
    #   "type": "image",
    #   "x": 0,
    #   "y": 0,
    #   "anchor": "tl",
    #   "width": 540,
    #   "height": 540,
    #   "level": 1,
    #   "inverted": false,
    #   "filled": true,
    #   "text": "",
    #   "name": "artist_name_track_title_or_icon_name",
    #   "endpoint": "album-art",
    #   "callback": "/api/image/8"
    # },



def parse_layout_data(layout_data):
    sp = get_spotify_client()
    track_info = get_current_track_info(sp)
    time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    for element in layout_data['elements']:
        if element['type'] == 'image':
            if element['endpoint'] == 'album-art':
                if track_info is None:
                    element['name'] = "None"
                else:
                    #name is used as the filename on the sd card
                    name = f"{track_info['artists'][0]['name']}_{track_info['album']}"
                    name = name[:16]
                    name = ''.join(c if c.isalnum() or c == '_' else '_' for c in name)
                    name = name.encode('ascii', 'ignore').decode('ascii')
                    name = name.lower()
                    element['name'] = name


        if element['type'] == 'text':
                if element['text'] == 'track_info':
                    if track_info is None:
                        element['text'] = " "
                    else:
                        element['text'] = f"{track_info['track']} by {track_info['artists'][0]['name']}"

                if element['text'] == 'track_title':
                    if track_info is None:
                        element['text'] = "No track"
                    else:
                        element['text'] = track_info['track']

                if element['text'] == 'track_artist':
                    if track_info is None:
                        element['text'] = ""
                    else:
                        element['text'] = track_info['artists'][0]['name']

                if element['text'] == 'time':
                    element['text'] = time


    return layout_data


@pages.route('/<layout_name>', methods=['GET'])
def get_layout_data(layout_name):
    print(f"Getting layout data for {layout_name}")
    layout_data = get_layout(layout_name)
    parsed_layout_data = parse_layout_data(layout_data) 
    return jsonify(parsed_layout_data)