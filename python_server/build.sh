#!/bin/bash
set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN} Starting build process...${NC}"

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
cd react-screen-creator || handle_error $LINENO
npm install --quiet || handle_error $LINENO
npm run build || handle_error $LINENO
cd ..

# Set up Python environment
echo -e "${GREEN} Setting up Python environment...${NC}"

if [ ! -d "python_env" ]; then
    echo "Creating new Python virtual environment..."
    $PYTHON_CMD -m venv python_env || handle_error $LINENO
fi

# Ensure we're using the virtual environment
VENV_PATH="$(pwd)/python_env"
source "$VENV_PATH/bin/activate" || handle_error $LINENO

# Verify we're in the virtual environment
if ! echo "$VIRTUAL_ENV" | grep -q "python_env"; then
    echo -e "${RED} Failed to activate virtual environment${NC}"
    exit 1
fi

echo -e "${GREEN} Virtual environment activated at: $VIRTUAL_ENV${NC}"

# Update pip and install requirements
echo -e "${GREEN} Installing Python dependencies...${NC}"
python -m pip install --upgrade pip || handle_error $LINENO
python -m pip install -r requirements.txt || handle_error $LINENO

# Start Flask server
echo -e "${GREEN} Starting Flask server...${NC}"
export FLASK_DEBUG=1 # Enable debug mode
python -m flask run --host=0.0.0.0

# Cleanup function
cleanup() {
    echo -e "${GREEN} Cleaning up...${NC}"
    deactivate 2>/dev/null || true
}

# Set up cleanup on script exit
trap cleanup EXIT