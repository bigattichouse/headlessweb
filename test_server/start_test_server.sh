#!/bin/bash

# Start the upload test server for HeadlessWeb testing

cd "$(dirname "$0")"

echo "Starting HeadlessWeb Upload Test Server on port 9876..."
echo "Server will be available at: http://localhost:9876"
echo "Press Ctrl+C to stop the server"
echo ""

# Check if node_modules exists
if [ ! -d "node_modules" ]; then
    echo "Installing dependencies..."
    npm install
    echo ""
fi

# Start the server
node upload-server.js