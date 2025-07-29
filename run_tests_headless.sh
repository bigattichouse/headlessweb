#!/bin/bash

# HeadlessWeb Test Runner with GTK Headless Configuration
# This script prevents GTK file dialogs and desktop integration during testing

echo "Starting HeadlessWeb tests in headless mode..."

# Set environment variables to prevent GTK desktop integration
export GDK_BACKEND=broadway
export GTK_RECENT_FILES_ENABLED=0
export GTK_RECENT_FILES_MAX_AGE=0
export WEBKIT_DISABLE_COMPOSITING_MODE=1
export WEBKIT_DISABLE_DMABUF_RENDERER=1
export XDG_CONFIG_HOME=/tmp/headless_gtk_config
export XDG_DATA_HOME=/tmp/headless_gtk_data

# Create temporary directories
mkdir -p /tmp/headless_gtk_config
mkdir -p /tmp/headless_gtk_data

# Disable GTK settings daemon
export GTK_MODULES=""

# Use virtual display if available (optional)
if command -v Xvfb >/dev/null 2>&1; then
    export DISPLAY=:99
    echo "Using virtual display :99"
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

echo "Test run completed"