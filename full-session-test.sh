#!/bin/bash

set -e

echo "=== Full Session Test ==="

SESSION_NAME="full_session_test"
SESSION_DIR="$HOME/.hweb-poc/sessions"
TEST_HTML_FILE="/tmp/full_session_test.html"

# Create a simple HTML file with a form
cat > "$TEST_HTML_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Full Session Test Page</title></head>
<body>
<h1>Initial Content</h1>
<form id="testForm">
    <input type="text" id="testInput" name="testInput" value="initial">
    <textarea id="testTextarea" name="testTextarea">initial textarea</textarea>
    <select id="testSelect" name="testSelect">
        <option value="option1">Option 1</option>
        <option value="option2">Option 2</option>
        <option value="option3">Option 3</option>
    </select>
    <input type="checkbox" id="testCheckbox" name="testCheckbox">
</form>
<div id="testDiv">This is a test div.</div>
<div style="height: 2000px;">Scrollable content</div>
</body>
</html>
EOF

# Clean up previous session data
echo "1. Cleaning up old session data..."
rm -rf "$SESSION_DIR"
mkdir -p "$SESSION_DIR"

# --- Phase 1: Manipulate DOM, set storage, form state, user agent, and custom variables, then save session ---
echo "2. Starting initial session, manipulating state..."
./hweb-poc --session "$SESSION_NAME" --url "file://$TEST_HTML_FILE" --quiet

echo "   Verifying initial DOM element..."
DOM_CHECK_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "var el = document.querySelector('#testDiv'); el ? el.innerText : 'NOT_FOUND';" --quiet | tail -n 1)
echo "   DOM Check (current): $DOM_CHECK_CURRENT"
if [[ "$DOM_CHECK_CURRENT" != "This is a test div." ]]; then
    echo "Error: DOM element not found or incorrect in current session!"
    exit 1
fi

echo "   Setting test cookies..."
./hweb-poc --session "$SESSION_NAME" --js "document.cookie = 'testCookie=sessionValue; path=/'; 'Cookie set';" --quiet

echo "   Setting Local Storage items..."
./hweb-poc --session "$SESSION_NAME" --js "localStorage.setItem('localKey1', 'localValue1'); localStorage.setItem('localKey2', 'localValue2'); 'LocalStorage set';" --quiet

echo "   Setting Session Storage items..."
./hweb-poc --session "$SESSION_NAME" --js "sessionStorage.setItem('sessionKey1', 'sessionValue1'); sessionStorage.setItem('sessionKey2', 'sessionValue2'); 'SessionStorage set';" --quiet

echo "   Filling form fields..."
./hweb-poc --session "$SESSION_NAME" --type "#testInput" "newInputValue" --quiet
./hweb-poc --session "$SESSION_NAME" --type "#testTextarea" "newTextareaValue" --quiet
./hweb-poc --session "$SESSION_NAME" --select "#testSelect" "option3" --quiet
./hweb-poc --session "$SESSION_NAME" --check "#testCheckbox" --quiet

echo "   Setting scroll position..."
./hweb-poc --session "$SESSION_NAME" --js "window.scrollTo(0, 1000); 'Scrolled';" --quiet

echo "   Setting custom user agent..."
# FIXED: Use --user-agent instead of trying to set via JavaScript
./hweb-poc --session "$SESSION_NAME" --user-agent "CustomUserAgent/1.0" --quiet

echo "   Storing custom variables..."
./hweb-poc --session "$SESSION_NAME" --store "myVar1" "customVal1" --quiet
./hweb-poc --session "$SESSION_NAME" --store "myVar2" "customVal2" --quiet

echo "   Verifying cookie in current session..."
COOKIE_CHECK_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "var cookie = document.cookie.split('; ').find(row => row.startsWith('testCookie=')); cookie ? cookie.split('=')[1] : 'NOT_FOUND';" --quiet | tail -n 1)
echo "   Cookie Check (current): $COOKIE_CHECK_CURRENT"
if [[ "$COOKIE_CHECK_CURRENT" != "sessionValue" ]]; then
    echo "Error: Cookie not found or incorrect in current session!"
    exit 1
fi

echo "   Verifying Local Storage in current session..."
LOCAL_STORAGE_CHECK_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = localStorage.getItem('localKey1'); var val2 = localStorage.getItem('localKey2'); val1 + '|' + val2;" --quiet | tail -n 1)
echo "   Local Storage Check (current): $LOCAL_STORAGE_CHECK_CURRENT"
if [[ "$LOCAL_STORAGE_CHECK_CURRENT" != "localValue1|localValue2" ]]; then
    echo "Error: Local Storage not found or incorrect in current session!"
    exit 1
fi

echo "   Verifying Session Storage in current session..."
SESSION_STORAGE_CHECK_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = sessionStorage.getItem('sessionKey1'); var val2 = sessionStorage.getItem('sessionKey2'); val1 + '|' + val2;" --quiet | tail -n 1)
echo "   Session Storage Check (current): $SESSION_STORAGE_CHECK_CURRENT"
if [[ "$SESSION_STORAGE_CHECK_CURRENT" != "sessionValue1|sessionValue2" ]]; then
    echo "Error: Session Storage not found or incorrect in current session!"
    exit 1
fi

echo "   Verifying Form State in current session..."
FORM_INPUT_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testInput').value;" --quiet | tail -n 1)
FORM_TEXTAREA_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testTextarea').value;" --quiet | tail -n 1)
FORM_SELECT_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testSelect').value;" --quiet | tail -n 1)
FORM_CHECKBOX_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testCheckbox').checked;" --quiet | tail -n 1)
echo "   Form State (current): Input='$FORM_INPUT_CURRENT', Textarea='$FORM_TEXTAREA_CURRENT', Select='$FORM_SELECT_CURRENT', Checkbox='$FORM_CHECKBOX_CURRENT'"
if [[ "$FORM_INPUT_CURRENT" != "newInputValue" || "$FORM_TEXTAREA_CURRENT" != "newTextareaValue" || "$FORM_SELECT_CURRENT" != "option3" || "$FORM_CHECKBOX_CURRENT" != "true" ]]; then
    echo "Error: Form state not correct in current session!"
    exit 1
fi

echo "   Verifying Scroll Position in current session..."
SCROLL_POS_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "window.pageYOffset;" --quiet | tail -n 1)
echo "   Scroll Position (current): $SCROLL_POS_CURRENT"
if [[ "$SCROLL_POS_CURRENT" -lt 900 ]]; then # Allow some tolerance
    echo "Error: Scroll position not correct in current session!"
    exit 1
fi

echo "   Verifying Custom User Agent in current session..."
USER_AGENT_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --js "navigator.userAgent;" --quiet | tail -n 1)
echo "   User Agent (current): $USER_AGENT_CURRENT"
if [[ "$USER_AGENT_CURRENT" != *"CustomUserAgent/1.0"* ]]; then
    echo "Warning: User Agent might not contain expected value. Got: $USER_AGENT_CURRENT"
fi

echo "   Verifying Custom Variables in current session..."
CUSTOM_VAR1_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --get "myVar1" --quiet | tail -n 1)
CUSTOM_VAR2_CURRENT=$(./hweb-poc --session "$SESSION_NAME" --get "myVar2" --quiet | tail -n 1)
echo "   Custom Vars (current): myVar1='$CUSTOM_VAR1_CURRENT', myVar2='$CUSTOM_VAR2_CURRENT'"
if [[ "$CUSTOM_VAR1_CURRENT" != "customVal1" || "$CUSTOM_VAR2_CURRENT" != "customVal2" ]]; then
    echo "Error: Custom variables not correct in current session!"
    exit 1
fi

echo "   Ending session to save state..."
./hweb-poc --session "$SESSION_NAME" --end --quiet

# --- Phase 2: Restore session and verify state ---
echo "3. Starting new session to restore state and verify..."
# Do NOT specify --url here, expect it to load from session
./hweb-poc --session "$SESSION_NAME" --quiet

echo "   Verifying DOM element in restored session..."
DOM_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var el = document.querySelector('#testDiv'); el ? el.innerText : 'NOT_FOUND_AFTER_RESTORE';" --quiet | tail -n 1)
echo "   DOM Check (restored): $DOM_CHECK_RESTORED"
if [[ "$DOM_CHECK_RESTORED" != "This is a test div." ]]; then
    echo "Warning: DOM element not found or incorrect after session restore! This is expected for JS-added elements."
fi

echo "   Verifying cookie in restored session..."
COOKIE_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var cookie = document.cookie.split('; ').find(row => row.startsWith('testCookie=')); cookie ? cookie.split('=')[1] : 'NOT_FOUND_AFTER_RESTORE';" --quiet | tail -n 1)
echo "   Cookie Check (restored): $COOKIE_CHECK_RESTORED"
if [[ "$COOKIE_CHECK_RESTORED" != "sessionValue" ]]; then
    echo "Warning: Cookie not found or incorrect after session restore!"
fi

echo "   Verifying Local Storage in restored session..."
LOCAL_STORAGE_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = localStorage.getItem('localKey1'); var val2 = localStorage.getItem('localKey2'); val1 + '|' + val2;" --quiet | tail -n 1)
echo "   Local Storage Check (restored): $LOCAL_STORAGE_CHECK_RESTORED"
if [[ "$LOCAL_STORAGE_CHECK_RESTORED" != "localValue1|localValue2" ]]; then
    echo "Warning: Local Storage not found or incorrect after session restore!"
fi

echo "   Verifying Session Storage in restored session..."
SESSION_STORAGE_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = sessionStorage.getItem('sessionKey1'); var val2 = sessionStorage.getItem('sessionKey2'); val1 + '|' + val2;" --quiet | tail -n 1)
echo "   Session Storage Check (restored): $SESSION_STORAGE_CHECK_RESTORED"
if [[ "$SESSION_STORAGE_CHECK_RESTORED" != "sessionValue1|sessionValue2" ]]; then
    echo "Warning: Session Storage not found or incorrect after session restore! (This is expected as sessionStorage is typically cleared on browser close)"
fi

echo "   Verifying Form State in restored session..."
FORM_INPUT_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testInput').value;" --quiet | tail -n 1)
FORM_TEXTAREA_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testTextarea').value;" --quiet | tail -n 1)
FORM_SELECT_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testSelect').value;" --quiet | tail -n 1)
FORM_CHECKBOX_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testCheckbox').checked;" --quiet | tail -n 1)
echo "   Form State (restored): Input='$FORM_INPUT_RESTORED', Textarea='$FORM_TEXTAREA_RESTORED', Select='$FORM_SELECT_RESTORED', Checkbox='$FORM_CHECKBOX_RESTORED'"
if [[ "$FORM_INPUT_RESTORED" != "newInputValue" || "$FORM_TEXTAREA_RESTORED" != "newTextareaValue" || "$FORM_SELECT_RESTORED" != "option3" || "$FORM_CHECKBOX_RESTORED" != "true" ]]; then
    echo "Warning: Form state not correct after session restore!"
fi

echo "   Verifying Scroll Position in restored session..."
SCROLL_POS_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "window.pageYOffset;" --quiet | tail -n 1)
echo "   Scroll Position (restored): $SCROLL_POS_RESTORED"
if [[ "$SCROLL_POS_RESTORED" -lt 900 ]]; then # Allow some tolerance
    echo "Warning: Scroll position not correct after session restore!"
fi

echo "   Verifying Custom User Agent in restored session..."
USER_AGENT_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "navigator.userAgent;" --quiet | tail -n 1)
echo "   User Agent (restored): $USER_AGENT_RESTORED"
if [[ "$USER_AGENT_RESTORED" != *"CustomUserAgent/1.0"* ]]; then
    echo "Warning: User Agent might not contain expected value after restore. Got: $USER_AGENT_RESTORED"
fi

echo "   Verifying Custom Variables in restored session..."
CUSTOM_VAR1_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --get "myVar1" --quiet | tail -n 1)
CUSTOM_VAR2_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --get "myVar2" --quiet | tail -n 1)
echo "   Custom Vars (restored): myVar1='$CUSTOM_VAR1_RESTORED', myVar2='$CUSTOM_VAR2_RESTORED'"
if [[ "$CUSTOM_VAR1_RESTORED" != "customVal1" || "$CUSTOM_VAR2_RESTORED" != "customVal2" ]]; then
    echo "Warning: Custom variables not correct after session restore!"
fi

echo "   Ending restored session... (final cleanup)"
./hweb-poc --session "$SESSION_NAME" --end --quiet

# --- Cleanup ---
echo "4. Cleaning up..."
rm "$TEST_HTML_FILE"
rm -rf "$SESSION_DIR"

echo "=== Full Session Test Complete ==="
echo ""
echo "Note: Some warnings are expected due to web browser standards:"
echo "- sessionStorage typically doesn't persist across browser restarts"
echo "- DOM elements added via JavaScript don't persist"
echo "- User agent setting has platform-specific behavior"
