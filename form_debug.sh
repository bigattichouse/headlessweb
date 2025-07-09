#!/bin/bash

set -e

SESSION_NAME="advanced_form_debug"
SESSION_DIR="$HOME/.hweb-poc/sessions"
SEARCH_QUERY="monster"

echo "--- Advanced Form Submission Analysis ---"

# Clean up
rm -rf "$SESSION_DIR"
mkdir -p "$SESSION_DIR"

# Initial visit
echo "1. Loading homepage..."
./hweb-poc --session "$SESSION_NAME" --url "https://limitless-adventures.com"

# Check form details and event handlers
echo "2. Analyzing form behavior..."
FORM_ANALYSIS_JS="
(function() {
  var form = document.querySelector('form');
  var input = document.querySelector('input[name=search]');
  var button = document.querySelector('button[name=dosearch]');
  
  var result = 'Form method: ' + (form.method || 'GET') + ' | ';
  result += 'Form action: ' + form.action + ' | ';
  result += 'Form onsubmit: ' + (form.onsubmit ? 'HAS_HANDLER' : 'NO_HANDLER') + ' | ';
  result += 'Button onclick: ' + (button.onclick ? 'HAS_HANDLER' : 'NO_HANDLER') + ' | ';
  result += 'Form has preventDefault: ' + (form.addEventListener ? 'POSSIBLE' : 'NO');
  
  return result;
})()
"
./hweb-poc --session "$SESSION_NAME" --js "$FORM_ANALYSIS_JS"

# Try direct form submission instead of button click
echo "3. Trying direct form submission..."
FORM_SUBMIT_JS="
(function() {
  var form = document.querySelector('form');
  var input = document.querySelector('input[name=search]');
  
  if (form && input) {
    input.value = '${SEARCH_QUERY}';
    
    // Try both button click AND form submit
    var button = document.querySelector('button[name=dosearch]');
    if (button) button.click();
    
    // Also try direct form submission
    form.submit();
    
    return 'Form submitted directly with value: ' + input.value;
  } else {
    return 'Form or input not found';
  }
})()
"
./hweb-poc --session "$SESSION_NAME" --js "$FORM_SUBMIT_JS"

# Check what happened after submission
echo "4. Checking page state after submission (waiting longer)..."
sleep 5  # Wait 5 seconds before checking

FINAL_STATE_JS="
(function() {
  var url = window.location.href;
  var title = document.title;
  var h1Text = document.querySelector('h1') ? document.querySelector('h1').innerText : 'NO_H1';
  var searchResults = document.querySelectorAll('.result, .product, .item, [class*=result], [class*=product]').length;
  
  return 'URL: ' + url + ' | Title: ' + title + ' | H1: ' + h1Text + ' | Potential results: ' + searchResults;
})()
"
./hweb-poc --session "$SESSION_NAME" --js "$FINAL_STATE_JS"

# Try accessing catalog.php directly to see if it exists
echo "5. Testing direct access to catalog.php..."
./hweb-poc --session "$SESSION_NAME" --url "https://limitless-adventures.com/catalog.php"

CATALOG_CHECK_JS="
(function() {
  var url = window.location.href;
  var title = document.title;
  var h1Text = document.querySelector('h1') ? document.querySelector('h1').innerText : 'NO_H1';
  
  return 'Direct catalog access - URL: ' + url + ' | Title: ' + title + ' | H1: ' + h1Text;
})()
"
./hweb-poc --session "$SESSION_NAME" --js "$CATALOG_CHECK_JS"

# Try with search parameter
echo "6. Testing catalog.php with search parameter..."
./hweb-poc --session "$SESSION_NAME" --url "https://limitless-adventures.com/catalog.php?search=${SEARCH_QUERY}"

CATALOG_SEARCH_JS="
(function() {
  var url = window.location.href;
  var title = document.title;
  var h1Text = document.querySelector('h1') ? document.querySelector('h1').innerText : 'NO_H1';
  var hasResults = document.body.innerText.toLowerCase().includes('${SEARCH_QUERY}'.toLowerCase());
  
  return 'Catalog with search - URL: ' + url + ' | Title: ' + title + ' | H1: ' + h1Text + ' | Has monster results: ' + hasResults;
})()
"
./hweb-poc --session "$SESSION_NAME" --js "$CATALOG_SEARCH_JS"

# Cleanup
echo "7. Cleaning up..."
./hweb-poc --session "$SESSION_NAME" --end

echo "--- Advanced Analysis Complete ---"
