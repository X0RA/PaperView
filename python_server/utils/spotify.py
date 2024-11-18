import spotipy
from spotipy.oauth2 import SpotifyOAuth
import time
import os

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

def get_current_track_info(sp):
    # Static variable to store cache and timestamp
    if not hasattr(get_current_track_info, '_cache'):
        get_current_track_info._cache = {'data': None, 'timestamp': 0}
    
    # Check if cache is valid (less than 5 seconds old)
    current_time = time.time()
    if (current_time - get_current_track_info._cache['timestamp']) < 5 and get_current_track_info._cache['data'] is not None:
        print("Using cached track info")
        return get_current_track_info._cache['data']
    
    print("Fetching fresh track info from Spotify API")
    current_playback = sp.current_playback()
    if current_playback is None or current_playback.get('item') is None:
        get_current_track_info._cache['data'] = None
        get_current_track_info._cache['timestamp'] = current_time
        return None
        
    track = current_playback['item']
    album_art = track['album']['images'][0]['url'] if track['album']['images'] else None
    
    track_info = {
        'id': track['id'],
        'track': track['name'],
        'artists': track['artists'],
        'album': track['album']['name'],
        'album_art': album_art,
        'is_playing': current_playback['is_playing']
    }
    
    # Update cache
    get_current_track_info._cache['data'] = track_info
    get_current_track_info._cache['timestamp'] = current_time
    
    return track_info

def toggle_playback_state(sp):
    current_playback = sp.current_playback()
    if current_playback is None:
        raise ValueError('No active device found')
        
    if current_playback['is_playing']:
        sp.pause_playback()
    else:
        sp.start_playback()
    
    return {'success': True, 'is_playing': not current_playback['is_playing']}

def skip_to_next(sp):
    sp.next_track()
    return {'success': True}

def skip_to_previous(sp):
    sp.previous_track()
    return {'success': True}


def get_track_liked_status(sp, track_id):
    return sp.current_user_saved_tracks_contains(track_id)
