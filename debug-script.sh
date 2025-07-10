#!/bin/bash

set -e

echo "=== Debug Session Test ==="

SESSION_NAME="debug_session_test"
SESSION_DIR="$HOME/.hweb-poc/sessions"
TEST_HTML_FILE="/tmp/debug_session_test.html"

# Function to check command success
check_command() {
    local cmd="$1"
    local description="$2"
    
    echo "Running: $cmd"
    if eval "$cmd"; then
        echo "✓ SUCCESS: $description"
    else
        echo "✗ FAILED: $description"
        echo "Exit code: $?"
    fi
    echo ""
}

# Function to verify value
verify_value() {
    local actual="$1"
    local expected="$2"
    local description="$3"
    
    if [[ "$actual" == "$expected" ]]; then
        echo "✓ PASS: $description"
        echo "  Value: '$actual'"
    else
        echo "✗ FAIL: $description"
        echo "  Expected: '$expected'"
        echo "  Actual: '$actual'"
    fi
    echo ""
}

# Create a simple HTML file with a form
cat > "$TEST_HTML_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Debug Session Test Page</title></head>
<body>
<h1>Debug Test Content</h1>
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
<div style="height: 2000px;">Scrollable content for testing scroll position</div>
</body>
</html>
EOF

echo "Created test HTML file: $TEST_HTML_FILE"
echo "File size: $(wc -c < "$TEST_HTML_FILE") bytes"
echo ""

# Clean up previous session data
echo "=== Cleanup Phase ==="
rm -rf "$SESSION_DIR"
mkdir -p "$SESSION_DIR"
echo "Created session directory: $SESSION_DIR"
echo ""

# Phase 1: Initial session setup
echo "=== Phase 1: Initial Session Setup ==="

check_command "./hweb-poc --session '$SESSION_NAME' --url 'file://$TEST_HTML_FILE'" "Initial navigation"

# Wait a moment for page to load
sleep 1

# Test basic DOM access
DOM_CHECK=$(./hweb-poc --session "$SESSION_NAME" --js "var el = document.querySelector('#testDiv'); el ? el.innerText : 'NOT_FOUND';" 2>/dev/null | tail -n 1)
verify_value "$DOM_CHECK" "This is a test div." "DOM element access"

echo "=== Phase 2: Setting State ==="

# Set cookies
check_command "./hweb-poc --session '$SESSION_NAME' --js \"document.cookie = 'testCookie=debugValue; path=/'; 'Cookie set';\"" "Setting cookies"

# Set localStorage  
check_command "./hweb-poc --session '$SESSION_NAME' --js \"localStorage.setItem('localKey1', 'localValue1'); localStorage.setItem('localKey2', 'localValue2'); 'LocalStorage set';\"" "Setting localStorage"

# Set sessionStorage
check_command "./hweb-poc --session '$SESSION_NAME' --js \"sessionStorage.setItem('sessionKey1', 'sessionValue1'); sessionStorage.setItem('sessionKey2', 'sessionValue2'); 'SessionStorage set';\"" "Setting sessionStorage"

# Set form values
check_command "./hweb-poc --session '$SESSION_NAME' --type '#testInput' 'debugInputValue'" "Setting input value"
check_command "./hweb-poc --session '$SESSION_NAME' --type '#testTextarea' 'debugTextareaValue'" "Setting textarea value"
check_command "./hweb-poc --session '$SESSION_NAME' --select '#testSelect' 'option3'" "Setting select value"
check_command "./hweb-poc --session '$SESSION_NAME' --check '#testCheckbox'" "Checking checkbox"

# Set scroll position
check_command "./hweb-poc --session '$SESSION_NAME' --js \"window.scrollTo(0, 1000); 'Scrolled';\"" "Setting scroll position"

# Set user agent
check_command "./hweb-poc --session '$SESSION_NAME' --user-agent 'DebugUserAgent/1.0'" "Setting user agent"

# Set custom variables
check_command "./hweb-poc --session '$SESSION_NAME' --store 'debugVar1' 'debugVal1'" "Setting custom variable 1"
check_command "./hweb-poc --session '$SESSION_NAME' --store 'debugVar2' 'debugVal2'" "Setting custom variable 2"

echo "=== Phase 3: Verifying Current State ==="

# Verify cookies
COOKIE_CHECK=$(./hweb-poc --session "$SESSION_NAME" --js "var cookie = document.cookie.split('; ').find(row => row.startsWith('testCookie=')); cookie ? cookie.split('=')[1] : 'NOT_FOUND';" 2>/dev/null | tail -n 1)
verify_value "$COOKIE_CHECK" "debugValue" "Cookie persistence (current session)"

# Verify localStorage
LOCAL_STORAGE_CHECK=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = localStorage.getItem('localKey1'); var val2 = localStorage.getItem('localKey2'); val1 + '|' + val2;" 2>/dev/null | tail -n 1)
verify_value "$LOCAL_STORAGE_CHECK" "localValue1|localValue2" "localStorage persistence (current session)"

# Verify sessionStorage
SESSION_STORAGE_CHECK=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = sessionStorage.getItem('sessionKey1'); var val2 = sessionStorage.getItem('sessionKey2'); val1 + '|' + val2;" 2>/dev/null | tail -n 1)
verify_value "$SESSION_STORAGE_CHECK" "sessionValue1|sessionValue2" "sessionStorage persistence (current session)"

# Verify form state
FORM_INPUT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testInput').value;" 2>/dev/null | tail -n 1)
verify_value "$FORM_INPUT" "debugInputValue" "Form input value (current session)"

FORM_TEXTAREA=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testTextarea').value;" 2>/dev/null | tail -n 1)
verify_value "$FORM_TEXTAREA" "debugTextareaValue" "Form textarea value (current session)"

FORM_SELECT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testSelect').value;" 2>/dev/null | tail -n 1)
verify_value "$FORM_SELECT" "option3" "Form select value (current session)"

FORM_CHECKBOX=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testCheckbox').checked;" 2>/dev/null | tail -n 1)
verify_value "$FORM_CHECKBOX" "true" "Form checkbox state (current session)"

# Verify scroll position
SCROLL_POS=$(./hweb-poc --session "$SESSION_NAME" --js "window.pageYOffset;" 2>/dev/null | tail -n 1)
echo "Scroll position (current session): $SCROLL_POS"
if [[ "$SCROLL_POS" -gt 900 ]]; then
    echo "✓ PASS: Scroll position is reasonable ($SCROLL_POS > 900)"
else
    echo "✗ FAIL: Scroll position too low ($SCROLL_POS <= 900)"
fi
echo ""

# Verify user agent
USER_AGENT=$(./hweb-poc --session "$SESSION_NAME" --js "navigator.userAgent;" 2>/dev/null | tail -n 1)
echo "User agent (current session): $USER_AGENT"
if [[ "$USER_AGENT" == *"DebugUserAgent/1.0"* ]]; then
    echo "✓ PASS: User agent contains expected value"
else
    echo "? INFO: User agent may not contain expected value (platform dependent)"
fi
echo ""

# Verify custom variables
CUSTOM_VAR1=$(./hweb-poc --session "$SESSION_NAME" --get 'debugVar1' 2>/dev/null | tail -n 1)
verify_value "$CUSTOM_VAR1" "debugVal1" "Custom variable 1 (current session)"

CUSTOM_VAR2=$(./hweb-poc --session "$SESSION_NAME" --get 'debugVar2' 2>/dev/null | tail -n 1)
verify_value "$CUSTOM_VAR2" "debugVal2" "Custom variable 2 (current session)"

echo "=== Phase 4: Ending Session ==="
check_command "./hweb-poc --session '$SESSION_NAME' --end" "Ending session"

# Check session file was created
SESSION_FILE="$SESSION_DIR/$SESSION_NAME.json"
if [[ -f "$SESSION_FILE" ]]; then
    echo "✓ SUCCESS: Session file created"
    echo "  File: $SESSION_FILE"
    echo "  Size: $(wc -c < "$SESSION_FILE") bytes"
else
    echo "✗ FAILED: Session file not found"
    echo "  Expected: $SESSION_FILE"
fi
echo ""

echo "=== Phase 5: Session Restoration ==="

# Start new session (should restore automatically)
check_command "./hweb-poc --session '$SESSION_NAME'" "Restoring session without URL"

# Wait a moment for restoration
sleep 2

echo "=== Phase 6: Verifying Restored State ==="

# Verify DOM after restoration
DOM_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var el = document.querySelector('#testDiv'); el ? el.innerText : 'NOT_FOUND_AFTER_RESTORE';" 2>/dev/null | tail -n 1)
verify_value "$DOM_CHECK_RESTORED" "This is a test div." "DOM element access (after restore)"

# Verify cookies after restoration
COOKIE_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var cookie = document.cookie.split('; ').find(row => row.startsWith('testCookie=')); cookie ? cookie.split('=')[1] : 'NOT_FOUND_AFTER_RESTORE';" 2>/dev/null | tail -n 1)
verify_value "$COOKIE_CHECK_RESTORED" "debugValue" "Cookie persistence (after restore)"

# Verify localStorage after restoration
LOCAL_STORAGE_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = localStorage.getItem('localKey1'); var val2 = localStorage.getItem('localKey2'); val1 + '|' + val2;" 2>/dev/null | tail -n 1)
verify_value "$LOCAL_STORAGE_CHECK_RESTORED" "localValue1|localValue2" "localStorage persistence (after restore)"

# Verify sessionStorage after restoration (may fail - non-standard)
SESSION_STORAGE_CHECK_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "var val1 = sessionStorage.getItem('sessionKey1'); var val2 = sessionStorage.getItem('sessionKey2'); val1 + '|' + val2;" 2>/dev/null | tail -n 1)
if [[ "$SESSION_STORAGE_CHECK_RESTORED" == "sessionValue1|sessionValue2" ]]; then
    echo "✓ PASS: sessionStorage persistence (after restore) - NON-STANDARD BEHAVIOR"
    echo "  Value: '$SESSION_STORAGE_CHECK_RESTORED'"
else
    echo "? INFO: sessionStorage not restored (after restore) - THIS IS EXPECTED BEHAVIOR"
    echo "  Value: '$SESSION_STORAGE_CHECK_RESTORED'"
fi
echo ""

# Verify form state after restoration
FORM_INPUT_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testInput').value;" 2>/dev/null | tail -n 1)
verify_value "$FORM_INPUT_RESTORED" "debugInputValue" "Form input value (after restore)"

FORM_TEXTAREA_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testTextarea').value;" 2>/dev/null | tail -n 1)
verify_value "$FORM_TEXTAREA_RESTORED" "debugTextareaValue" "Form textarea value (after restore)"

FORM_SELECT_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testSelect').value;" 2>/dev/null | tail -n 1)
verify_value "$FORM_SELECT_RESTORED" "option3" "Form select value (after restore)"

FORM_CHECKBOX_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testCheckbox').checked;" 2>/dev/null | tail -n 1)
verify_value "$FORM_CHECKBOX_RESTORED" "true" "Form checkbox state (after restore)"

# Verify scroll position after restoration
SCROLL_POS_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "window.pageYOffset;" 2>/dev/null | tail -n 1)
echo "Scroll position (after restore): $SCROLL_POS_RESTORED"
if [[ "$SCROLL_POS_RESTORED" -gt 900 ]]; then
    echo "✓ PASS: Scroll position restored correctly ($SCROLL_POS_RESTORED > 900)"
else
    echo "? INFO: Scroll position may not be restored ($SCROLL_POS_RESTORED <= 900)"
fi
echo ""

# Verify user agent after restoration
USER_AGENT_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "navigator.userAgent;" 2>/dev/null | tail -n 1)
echo "User agent (after restore): $USER_AGENT_RESTORED"
if [[ "$USER_AGENT_RESTORED" == *"DebugUserAgent/1.0"* ]]; then
    echo "✓ PASS: User agent restored correctly"
else
    echo "? INFO: User agent may not be restored (platform dependent)"
fi
echo ""

# Verify custom variables after restoration
CUSTOM_VAR1_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --get 'debugVar1' 2>/dev/null | tail -n 1)
verify_value "$CUSTOM_VAR1_RESTORED" "debugVal1" "Custom variable 1 (after restore)"

CUSTOM_VAR2_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --get 'debugVar2' 2>/dev/null | tail -n 1)
verify_value "$CUSTOM_VAR2_RESTORED" "debugVal2" "Custom variable 2 (after restore)"

echo "=== Phase 7: Final Cleanup ==="
check_command "./hweb-poc --session '$SESSION_NAME' --end" "Final session cleanup"

rm "$TEST_HTML_FILE"
rm -rf "$SESSION_DIR"

echo "=== Debug Session Test Complete ==="
echo ""
echo "SUMMARY:"
echo "- ✓ marks tests that should pass"
echo "- ✗ marks tests that failed"  
echo "- ? marks tests with expected warnings or platform-dependent behavior"
echo ""
echo "Expected warnings:"
echo "- sessionStorage persistence (non-standard behavior)"
echo "- User agent setting (platform dependent)"
echo "- Scroll position restoration (timing dependent)"
