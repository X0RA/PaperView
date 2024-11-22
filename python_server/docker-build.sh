#!/bin/bash
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Error handling function
handle_error() {
    echo -e "${RED} Error: Build failed at line $1${NC}"
    exit 1
}

# Set up error handling
trap 'handle_error $LINENO' ERR

echo -e "${GREEN} Starting build process...${NC}"
echo -e "${GREEN} Script directory: ${SCRIPT_DIR}${NC}"

# Build React app
echo -e "${GREEN} Building React application...${NC}"
REACT_APP_DIR="${SCRIPT_DIR}/react-screen-creator"

if [ ! -d "$REACT_APP_DIR" ]; then
    echo -e "${RED}React app directory not found at: ${REACT_APP_DIR}${NC}"
    echo "Current directory structure:"
    ls -la "${SCRIPT_DIR}"
    handle_error $LINENO
fi

cd "$REACT_APP_DIR" || handle_error $LINENO
npm install --quiet || handle_error $LINENO
npm run build || handle_error $LINENO
cd "$SCRIPT_DIR" || handle_error $LINENO

# Install Python dependencies directly (no virtual env needed in Docker)
echo -e "${GREEN} Installing Python dependencies...${NC}"
if [ ! -f "${SCRIPT_DIR}/requirements.txt" ]; then
    echo -e "${RED}requirements.txt not found at: ${SCRIPT_DIR}/requirements.txt${NC}"
    echo "Current directory structure:"
    ls -la "${SCRIPT_DIR}"
    handle_error $LINENO
fi

python -m pip install --upgrade pip || handle_error $LINENO
python -m pip install -r "${SCRIPT_DIR}/requirements.txt" || handle_error $LINENO

# Start Flask server for Docker
echo -e "${GREEN} Starting Flask server...${NC}"
if [ ! -f "${SCRIPT_DIR}/server.py" ]; then
    echo -e "${RED}server.py not found at: ${SCRIPT_DIR}/server.py${NC}"
    echo "Current directory structure:"
    ls -la "${SCRIPT_DIR}"
    handle_error $LINENO
fi

export FLASK_APP="${SCRIPT_DIR}/server.py"
cd "${SCRIPT_DIR}" || handle_error $LINENO
python -m flask run --host=0.0.0.0 --port=5000