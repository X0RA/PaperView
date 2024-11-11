import json
import os
from datetime import datetime
from flask import Blueprint, request, jsonify
from pathlib import Path
import requests

layout = Blueprint('layout', __name__)

LAYOUTS_DIR = 'layouts'

def ensure_layout_directory():
    Path(LAYOUTS_DIR).mkdir(exist_ok=True)

@layout.route('/save-layout', methods=['POST'])
def save_layout():
    try:
        layout_data = request.get_json()

        if not layout_data:
            return jsonify({
                'success': False,
                'message': 'No data received'
            }), 400

        ensure_layout_directory()

        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        filename = f'layout_{timestamp}.json'
        file_path = os.path.join(LAYOUTS_DIR, filename)
        
        save_data = {
            'elements': layout_data,
            'metadata': {
                'created_at': datetime.now().isoformat(),
                'filename': filename
            }
        }

        # Write the JSON data to file
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(save_data, f, indent=2, ensure_ascii=False)

        # Also save as latest.json
        latest_path = os.path.join(LAYOUTS_DIR, 'latest.json')
        with open(latest_path, 'w', encoding='utf-8') as f:
            json.dump(save_data, f, indent=2, ensure_ascii=False)

        # Trigger display refresh
        try:
            refresh_response = requests.post('http://192.168.1.8/refresh', timeout=5)
            refresh_status = 'Display refresh successful' if refresh_response.status_code == 200 else 'Display refresh failed'
        except requests.exceptions.RequestException as e:
            refresh_status = f'Display refresh error: {str(e)}'
            print(f"Error refreshing display: {str(e)}")
        
        # Return success response
        return jsonify({
            'success': True,
            'message': f'Layout saved successfully. {refresh_status}',
            'path': file_path,
            'filename': filename
        }), 200

    except Exception as e:
        print(f"Error saving layout: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Error saving layout: {str(e)}'
        }), 500
        
@layout.route('/get-layout', methods=['GET'])
def get_layout():
    try:
        latest_path = os.path.join(LAYOUTS_DIR, 'latest.json')
        
        if not os.path.exists(latest_path):
            return jsonify({
                'success': False,
                'message': 'No layout found'
            }), 404

        with open(latest_path, 'r', encoding='utf-8') as f:
            layout_data = json.load(f)

        return jsonify({
            'success': True,
            'layout': layout_data
        }), 200

    except Exception as e:
        return jsonify({
            'success': False,
            'message': f'Error loading layout: {str(e)}'
        }), 500

@layout.route('/list-layout', methods=['GET'])
def list_layouts():
    try:
        ensure_layout_directory()
        layouts = []
        
        for file in os.listdir(LAYOUTS_DIR):
            if file.endswith('.json') and file != 'latest.json':
                file_path = os.path.join(LAYOUTS_DIR, file)
                with open(file_path, 'r', encoding='utf-8') as f:
                    layout_data = json.load(f)
                layouts.append({
                    'filename': file,
                    'created_at': layout_data.get('metadata', {}).get('created_at'),
                    'path': file_path
                })

        return jsonify({
            'success': True,
            'layouts': sorted(layouts, key=lambda x: x['filename'], reverse=True)
        }), 200

    except Exception as e:
        return jsonify({
            'success': False,
            'message': f'Error listing layouts: {str(e)}'
        }), 500