import spotipy
from spotipy.oauth2 import SpotifyOAuth
import os

os.environ['SPOTIPY_CLIENT_ID'] = 'c6a9ec79cc81470d88656fd7d3c84f5d'
os.environ['SPOTIPY_CLIENT_SECRET'] = '33af11922cfd4587b71540d04a21c874'
os.environ['SPOTIPY_REDIRECT_URI'] = 'http://localhost:8888/callback'


def get_spotify_client():
    """Initialize and return a Spotify client"""
    cache_path = "./cache"
    auth_manager = SpotifyOAuth(
        scope="user-read-playback-state user-modify-playback-state user-library-read",
        open_browser=False,
        cache_path=cache_path
    )
    return spotipy.Spotify(auth_manager=auth_manager)

def get_track_info():
    """Get current track name and artist information"""
    try:
        sp = get_spotify_client()
        current_playback = sp.current_playback()
        
        if current_playback is None or current_playback.get('item') is None:
            return {
                'id': 0,
                'artist': "",
                'title': "Nothing playing"
            }
            
        track = current_playback['item']
        return {
            'id': track['id'],
            'artist': ', '.join([artist['name'] for artist in track['artists']]),
            'title': track['name']
        }
    except Exception as e:
        print(f"Error getting track info: {str(e)}")
        return None

def get_album_art():
    """Get the album art URL for the current track"""
    try:
        sp = get_spotify_client()
        current_playback = sp.current_playback()
        
        if current_playback is None or current_playback.get('item') is None:
            return None
            
        track = current_playback['item']
        if track['album']['images']:
            return {
                'url': track['album']['images'][0]['url']
            }
        return None
    except Exception as e:
        print(f"Error getting album art: {str(e)}")
        return None

def get_track_liked_status():
    """Check if the current track is liked by the user"""
    try:
        sp = get_spotify_client()
        current_playback = sp.current_playback()
        
        if current_playback is None or current_playback.get('item') is None:
            return None
            
        track = current_playback['item']
        track_id = track['id']
        
        # Check if track is saved in user's library
        is_liked = sp.current_user_saved_tracks_contains([track_id])[0]
        
        return {
            'id': track_id,
            'liked': is_liked
        }
    except Exception as e:
        print(f"Error checking liked status: {str(e)}")
        return None