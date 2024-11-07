import io
from flask import Blueprint, jsonify, request
import spotipy
from spotipy.oauth2 import SpotifyOAuth
import os

# Create blueprint
spotify = Blueprint('spotify', __name__)

# Spotify API credentials
os.environ['SPOTIPY_CLIENT_ID'] = 'c6a9ec79cc81470d88656fd7d3c84f5d'
os.environ['SPOTIPY_CLIENT_SECRET'] = '33af11922cfd4587b71540d04a21c874'
os.environ['SPOTIPY_REDIRECT_URI'] = 'http://localhost:8888/callback'

def get_spotify_client():
    cache_path = "./cache"
    auth_manager = SpotifyOAuth(
        scope="user-read-playback-state user-modify-playback-state",
        open_browser=False,
        cache_path=cache_path
    )
    return spotipy.Spotify(auth_manager=auth_manager)

@spotify.route('/current-track', methods=['GET'])
def get_current_track():
    try:
        sp = get_spotify_client()
        current_playback = sp.current_playback()
        
        print(current_playback)
        
        if current_playback is None or current_playback.get('item') is None:
            return jsonify({
                'error': 'No track currently playing'
            }), 404

        track = current_playback['item']
        artists = ', '.join([artist['name'] for artist in track['artists']])
        album_art = track['album']['images'][0]['url'] if track['album']['images'] else None

        return jsonify({
            'track': track['name'],
            'artist': artists,
            'album_art': album_art,
            'is_playing': current_playback['is_playing']
        })

    except Exception as e:
        print(f"Error getting current track: {str(e)}")
        return jsonify({'error': str(e)}), 500

@spotify.route('/playback/toggle', methods=['POST'])
def toggle_playback():
    try:
        sp = get_spotify_client()
        current_playback = sp.current_playback()
        
        if current_playback is None:
            return jsonify({'error': 'No active device found'}), 404

        if current_playback['is_playing']:
            sp.pause_playback()
        else:
            sp.start_playback()

        return jsonify({'success': True, 'is_playing': not current_playback['is_playing']})

    except Exception as e:
        print(f"Error toggling playback: {str(e)}")
        return jsonify({'error': str(e)}), 500

@spotify.route('/playback/next', methods=['POST'])
def next_track():
    try:
        sp = get_spotify_client()
        sp.next_track()
        return jsonify({'success': True})

    except Exception as e:
        print(f"Error skipping to next track: {str(e)}")
        return jsonify({'error': str(e)}), 500

@spotify.route('/playback/previous', methods=['POST'])
def previous_track():
    try:
        sp = get_spotify_client()
        sp.previous_track()
        return jsonify({'success': True})

    except Exception as e:
        print(f"Error going to previous track: {str(e)}")
        return jsonify({'error': str(e)}), 500