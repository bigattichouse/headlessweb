#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Define the session name
SESSION_NAME="limitless_search_test"

# Define the session directory
SESSION_DIR="$HOME/.hweb-poc/sessions"

# Define the search query
SEARCH_QUERY="monster"

# Function to run hweb-poc and filter warnings
run_hweb_poc() {
    ./hweb-poc "$@" 2> >(grep -v "GStreamer FDK AAC plugin is missing" >&2)
}

echo "--- Starting hweb-poc demonstration ---"

# 1. Clean up previous session data
echo "1. Cleaning up previous session data..."
rm -rf "$SESSION_DIR"
mkdir -p "$SESSION_DIR"

# 2. Initial Visit to Limitless Adventures and saving session
echo "2. Initial Visit to Limitless Adventures and saving session..."
run_hweb_poc --session "$SESSION_NAME" --url "https://limitless-adventures.com"

# 3. Entering text and submitting search
echo "3. Entering text and submitting search..."
run_hweb_poc --session "$SESSION_NAME" --js "document.querySelector('input[name=search]').value = '${SEARCH_QUERY}'; document.querySelector('button[name=dosearch]').click(); 'Search query entered and submitted';"

# 4. Re-opening the Session and Verifying the Page Content
echo "4. Re-opening session and verifying current URL..."
VERIFY_JS="
    var current_url = window.location.href;
    var url_contains_query = current_url.includes('search=${SEARCH_QUERY}');
    current_url + ' | URL contains query: ' + url_contains_query;
"
run_hweb_poc --session "$SESSION_NAME" --js "${VERIFY_JS}"

# 5. Clear the session at the end
echo "5. Clearing session '${SESSION_NAME}'..."
run_hweb_poc --session "$SESSION_NAME" --end

echo "--- hweb-poc demonstration completed successfully ---"
