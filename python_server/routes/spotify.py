from flask import Blueprint, jsonify
from utils.spotify import (
    get_spotify_client,
    get_current_track_info,
    toggle_playback_state,
    skip_to_next,
    skip_to_previous
)

spotify = Blueprint('spotify', __name__)

@spotify.route('/current-track', methods=['GET'])
def get_current_track():
    try:
        sp = get_spotify_client()
        track_info = get_current_track_info(sp)
        
        if track_info is None:
            return jsonify({
                'error': 'No track currently playing'
            }), 200
            
        return jsonify(track_info)
    except Exception as e:
        print(f"Error getting current track: {str(e)}")
        return jsonify({'error': str(e)}), 500

@spotify.route('/playback/toggle', methods=['POST'])
def toggle_playback():
    try:
        sp = get_spotify_client()
        result = toggle_playback_state(sp)
        return jsonify(result)
    except ValueError as ve:
        return jsonify({'error': str(ve)}), 404
    except Exception as e:
        print(f"Error toggling playback: {str(e)}")
        return jsonify({'error': str(e)}), 500

@spotify.route('/playback/next', methods=['POST'])
def next_track():
    try:
        sp = get_spotify_client()
        result = skip_to_next(sp)
        return jsonify(result)
    except Exception as e:
        print(f"Error skipping to next track: {str(e)}")
        return jsonify({'error': str(e)}), 500

@spotify.route('/playback/previous', methods=['POST'])
def previous_track():
    try:
        sp = get_spotify_client()
        result = skip_to_previous(sp)
        return jsonify(result)
    except Exception as e:
        print(f"Error going to previous track: {str(e)}")
        return jsonify({'error': str(e)}), 500