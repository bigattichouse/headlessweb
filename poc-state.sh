#!/bin/bash

set -e

SESSION_NAME="enhanced_test"
SESSION_DIR="$HOME/.hweb-poc/sessions"
SEARCH_QUERY="monster"

echo "--- Testing Enhanced Command Chaining ---"

# Clean up
echo "1. Cleaning up..."
rm -rf "$SESSION_DIR"
mkdir -p "$SESSION_DIR"

# Test the new command chaining approach
echo "2. Navigate and fill form with command chaining..."
./hweb-poc --session "$SESSION_NAME" \
    --url "https://limitless-adventures.com" \
    --wait "h1" \
    --type "input[name=search]" "$SEARCH_QUERY" \
    --click "button[name=dosearch]" \
    --wait-nav

echo "3. Check results..."
./hweb-poc --session "$SESSION_NAME" \
    --wait "h1" \
    --text "h1"

echo "4. Get current URL..."
./hweb-poc --session "$SESSION_NAME" \
    --js "(function() { return window.location.href; })()"

# Alternative: Test the smart search command
echo "5. Testing smart search command..."
./hweb-poc --session "$SESSION_NAME" \
    --url "https://limitless-adventures.com" \
    --search "$SEARCH_QUERY" \
    --wait "h1" \
    --text "h1"

# Cleanup
echo "6. Cleanup..."
./hweb-poc --session "$SESSION_NAME" --end

echo "--- Enhanced test completed ---"
