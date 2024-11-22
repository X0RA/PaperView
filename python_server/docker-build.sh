#!/bin/bash
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"  # Go up one level to project root

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN} Starting build process...${NC}"
echo -e "${GREEN} Script directory: ${SCRIPT_DIR}${NC}"
echo -e "${GREEN} Project root: ${PROJECT_ROOT}${NC}"

# Determine Python command
get_python_cmd() {
    if command -v python3 &> /dev/null; then
        echo "python3"
    elif command -v python &> /dev/null && [[ $(python --version 2>&1) == *"Python 3"* ]]; then
        echo "python"
    else
        echo -e "${RED} Python 3 not found${NC}"
        exit 1
    fi
}

PYTHON_CMD=$(get_python_cmd)
echo -e "${GREEN} Using Python command: ${PYTHON_CMD}${NC}"

# Function for error handling
handle_error() {
    echo -e "${RED} Error: Build failed at line $1${NC}"
    exit 1
}

# Set up error handling
trap 'handle_error $LINENO' ERR

# Build React app
echo -e "${GREEN} Building React application...${NC}"
REACT_APP_DIR="${PROJECT_ROOT}/react-screen-creator"

if [ ! -d "$REACT_APP_DIR" ]; then
    echo -e "${RED}React app directory not found at: ${REACT_APP_DIR}${NC}"
    echo "Current directory structure:"
    ls -la "${PROJECT_ROOT}"
    handle_error $LINENO
fi

cd "$REACT_APP_DIR" || handle_error $LINENO
npm install --quiet || handle_error $LINENO
npm run build || handle_error $LINENO
cd "$PROJECT_ROOT" || handle_error $LINENO

# Install Python dependencies directly (no virtual env needed in Docker)
echo -e "${GREEN} Installing Python dependencies...${NC}"
python -m pip install --upgrade pip || handle_error $LINENO
python -m pip install -r "${PROJECT_ROOT}/requirements.txt" || handle_error $LINENO

# Start Flask server for Docker
echo -e "${GREEN} Starting Flask server...${NC}"
# Remove FLASK_DEBUG for production
export FLASK_APP=app
# Use 0.0.0.0 to allow external connections and port 5000 to match docker-compose
cd "${SCRIPT_DIR}" || handle_error $LINENO  # Make sure we're in the Python server directory
python -m flask run --host=0.0.0.0 --port=5000