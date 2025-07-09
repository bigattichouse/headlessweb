#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Define the session name
SESSION_NAME="limitless_search_test"

# Define the session directory
SESSION_DIR="$HOME/.hweb-poc/sessions"

# Define the search query
SEARCH_QUERY="monster"

echo "--- Starting hweb-poc demonstration ---"

# 1. Clean up previous session data
echo "1. Cleaning up previous session data..."
rm -rf "$SESSION_DIR"
mkdir -p "$SESSION_DIR"

# 2. Initial Visit to Limitless Adventures and saving session
echo "2. Initial Visit to Limitless Adventures and saving session..."
./hweb-poc --session "$SESSION_NAME" --url "https://limitless-adventures.com"

# 3. Navigate directly to search results using the --url parameter
echo "3. Navigating directly to search results..."
./hweb-poc --session "$SESSION_NAME" --url "https://limitless-adventures.com/catalog.php?search=${SEARCH_QUERY}"

# 4. Re-opening the Session and Verifying the Page Content
echo "4. Re-opening session and verifying current URL..."
VERIFY_JS="
    var current_url = window.location.href;
    var url_contains_query = current_url.includes('search=${SEARCH_QUERY}') || current_url.includes('catalog.php');
    current_url + ' | URL contains query or catalog: ' + url_contains_query;
"
./hweb-poc --session "$SESSION_NAME" --js "${VERIFY_JS}"

# 5. Clear the session at the end
echo "5. Clearing session '${SESSION_NAME}'..."
./hweb-poc --session "$SESSION_NAME" --end

echo "--- hweb-poc demonstration completed successfully ---"
