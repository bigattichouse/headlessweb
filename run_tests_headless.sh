#!/bin/bash

# HeadlessWeb Test Runner with GTK Headless Configuration
# This script prevents GTK file dialogs and desktop integration during testing

echo "Starting HeadlessWeb tests in headless mode..."

# Set comprehensive environment variables to prevent GTK desktop integration
# Don't force a specific backend, let GTK choose the best available one
export GTK_RECENT_FILES_ENABLED=0
export GTK_RECENT_FILES_MAX_AGE=0
export WEBKIT_DISABLE_COMPOSITING_MODE=1
export WEBKIT_DISABLE_DMABUF_RENDERER=1
export XDG_CONFIG_HOME=/tmp/headless_gtk_config
export XDG_DATA_HOME=/tmp/headless_gtk_data
export XDG_RUNTIME_DIR=/tmp/headless_runtime
export TMPDIR=/tmp
export HOME=/tmp/headless_home

# Disable accessibility and other desktop features
export NO_AT_BRIDGE=1
export GTK_A11Y=none
export GTK_MODULES=""
export QT_QPA_PLATFORM=offscreen

# Additional file dialog prevention measures
export WEBKIT_DISABLE_FILE_PICKER=1
export GTK_FILE_CHOOSER_BACKEND=none
export GIO_USE_VFS=local
export GVFS_DISABLE_FUSE=1

# Create temporary directories
mkdir -p /tmp/headless_gtk_config
mkdir -p /tmp/headless_gtk_data
mkdir -p /tmp/headless_runtime
mkdir -p /tmp/headless_home

# Set up virtual display for proper headless operation
export DISPLAY=:99

# Start Xvfb if available and not already running
if command -v Xvfb >/dev/null 2>&1; then
    if ! pgrep -f "Xvfb :99" > /dev/null; then
        echo "Starting virtual display server..."
        Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
        XVFB_PID=$!
        sleep 2  # Give Xvfb time to start
        echo "Virtual display :99 started (PID: $XVFB_PID)"
    else
        echo "Virtual display :99 already running"
    fi
else
    echo "Xvfb not available, using offscreen backend"
fi

echo "Environment configured for headless operation"
echo "Running tests..."

# Run the actual tests
if [ -f "tests/hweb_tests" ]; then
    cd tests && ./hweb_tests "$@"
elif [ -f "hweb_tests" ]; then
    ./hweb_tests "$@"
else
    echo "Error: hweb_tests binary not found"
    echo "Please run 'make hweb_tests' first"
    exit 1
fi

# Cleanup
rm -rf /tmp/headless_gtk_config /tmp/headless_gtk_data

# Kill Xvfb if we started it
if [ ! -z "$XVFB_PID" ]; then
    echo "Stopping virtual display server (PID: $XVFB_PID)..."
    kill $XVFB_PID >/dev/null 2>&1
fi

echo "Test run completed"