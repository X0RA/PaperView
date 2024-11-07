import sys
import os
from flask import Flask
from routes.pages import pages
from routes.spotify import spotify




app = Flask(__name__)
app.register_blueprint(pages)
app.register_blueprint(spotify, url_prefix='/spotify')

if __name__ == '__main__':
    try:
        app.run(host='0.0.0.0', port=5000)
    except Exception as e:
        logging.error(f"Error starting server: {str(e)}")
        sys.exit(1)