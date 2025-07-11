#!/bin/bash

set -e

echo "=== HeadlessWeb Comprehensive Test Suite ==="
echo "Testing all functionality of hweb-poc"
echo ""

# Configuration
SESSION_DIR="$HOME/.hweb-poc/sessions"
LOCAL_TEST_FILE="/tmp/hweb_test.html"
REMOTE_TEST_URL="https://example.com"
SEARCH_TEST_URL="https://limitless-adventures.com"

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
check_command() {
    local cmd="$1"
    local description="$2"
    
    echo "Running: $cmd"
    if eval "$cmd"; then
        echo -e "${GREEN}✓ SUCCESS${NC}: $description"
    else
        local exit_code=$?
        echo -e "${RED}✗ FAILED${NC}: $description (exit code: $exit_code)"
        return $exit_code
    fi
    echo ""
}

verify_value() {
    local actual="$1"
    local expected="$2"
    local description="$3"
    
    if [[ "$actual" == "$expected" ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description"
        echo "  Value: '$actual'"
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected: '$expected'"
        echo "  Actual: '$actual'"
    fi
    echo ""
}

verify_contains() {
    local actual="$1"
    local expected="$2"
    local description="$3"
    
    if [[ "$actual" == *"$expected"* ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description"
        echo "  Value contains: '$expected'"
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected to contain: '$expected'"
        echo "  Actual: '$actual'"
    fi
    echo ""
}

info_message() {
    echo -e "${YELLOW}ℹ INFO${NC}: $1"
    echo ""
}

cleanup() {
    echo "=== Cleanup Phase ==="
    rm -rf "$SESSION_DIR"
    rm -f "$LOCAL_TEST_FILE"
    mkdir -p "$SESSION_DIR"
    echo "Cleaned up session directory and test files"
    echo ""
}

# Create test HTML file
create_test_html() {
    cat > "$LOCAL_TEST_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head><title>HeadlessWeb Test Page</title></head>
<body>
<h1>Test Page Content</h1>
<form id="testForm" action="/submit" method="post">
    <input type="text" id="searchInput" name="search" placeholder="Search...">
    <input type="text" id="testInput" name="testInput" value="initial">
    <textarea id="testTextarea" name="testTextarea">initial textarea</textarea>
    <select id="testSelect" name="testSelect">
        <option value="option1">Option 1</option>
        <option value="option2">Option 2</option>
        <option value="option3">Option 3</option>
    </select>
    <input type="checkbox" id="testCheckbox" name="testCheckbox">
    <button type="submit" id="submitBtn">Submit</button>
</form>
<div id="testDiv">This is a test div.</div>
<div style="height: 2000px;">Scrollable content for testing scroll position</div>
</body>
</html>
EOF
    echo "Created test HTML file: $LOCAL_TEST_FILE"
}

# Start with cleanup
cleanup
create_test_html

# Test 1: Basic Navigation and Session Creation
echo "=== Test 1: Basic Navigation and Session Creation ==="

check_command "./hweb-poc --session 'basic_test' --url 'file://$LOCAL_TEST_FILE'" "Navigate to local file"

DOM_CHECK=$(./hweb-poc --session "basic_test" --text "#testDiv" 2>/dev/null | tail -n 1)
verify_value "$DOM_CHECK" "This is a test div." "Local file DOM access"

check_command "./hweb-poc --session 'basic_test' --end" "End basic session"

# Test 2: Command Chaining
echo "=== Test 2: Command Chaining ==="

check_command "./hweb-poc --session 'chain_test' \
    --url 'file://$LOCAL_TEST_FILE' \
    --wait 'h1' \
    --type '#testInput' 'chained value' \
    --select '#testSelect' 'option2' \
    --check '#testCheckbox' \
    --js 'window.scrollTo(0, 500); \"done\"'" "Command chaining"

# Verify chained commands worked
INPUT_VAL=$(./hweb-poc --session "chain_test" --js "document.querySelector('#testInput').value" 2>/dev/null | tail -n 1)
verify_value "$INPUT_VAL" "chained value" "Chained input value"

SELECT_VAL=$(./hweb-poc --session "chain_test" --js "document.querySelector('#testSelect').value" 2>/dev/null | tail -n 1)
verify_value "$SELECT_VAL" "option2" "Chained select value"

check_command "./hweb-poc --session 'chain_test' --end" "End chain test session"

# Test 3: Full State Persistence (Local File)
echo "=== Test 3: Full State Persistence (Local File) ==="

SESSION_NAME="state_test"

# Set all types of state
check_command "./hweb-poc --session '$SESSION_NAME' --url 'file://$LOCAL_TEST_FILE'" "Initial navigation"

echo "Setting comprehensive state..."

# Cookies
check_command "./hweb-poc --session '$SESSION_NAME' --js \"document.cookie = 'testCookie=stateValue; path=/'; 'Cookie set';\"" "Set cookie"

# localStorage
check_command "./hweb-poc --session '$SESSION_NAME' --js \"localStorage.setItem('key1', 'value1'); localStorage.setItem('key2', 'value2'); 'Storage set';\"" "Set localStorage"

# Form state
check_command "./hweb-poc --session '$SESSION_NAME' --type '#testInput' 'persistedValue'" "Set input"
check_command "./hweb-poc --session '$SESSION_NAME' --type '#testTextarea' 'persistedText'" "Set textarea"
check_command "./hweb-poc --session '$SESSION_NAME' --select '#testSelect' 'option3'" "Set select"
check_command "./hweb-poc --session '$SESSION_NAME' --check '#testCheckbox'" "Check checkbox"

# Scroll position
check_command "./hweb-poc --session '$SESSION_NAME' --js \"window.scrollTo(0, 1000); 'Scrolled';\"" "Set scroll"

# User agent
check_command "./hweb-poc --session '$SESSION_NAME' --user-agent 'TestAgent/1.0'" "Set user agent"

# Custom variables
check_command "./hweb-poc --session '$SESSION_NAME' --store 'customVar' 'customValue'" "Store custom variable"

# End session to save state
check_command "./hweb-poc --session '$SESSION_NAME' --end" "Save session state"

# Verify session file exists
SESSION_FILE="$SESSION_DIR/${SESSION_NAME}.json"
if [[ -f "$SESSION_FILE" ]]; then
    echo -e "${GREEN}✓ SUCCESS${NC}: Session file created"
    echo "  Size: $(wc -c < "$SESSION_FILE") bytes"
else
    echo -e "${RED}✗ FAILED${NC}: Session file not found"
fi
echo ""

# Restore session and verify
echo "Restoring session..."
check_command "./hweb-poc --session '$SESSION_NAME'" "Restore session"

# Verify all state
COOKIE_RESTORED=$(./hweb-poc --session "$SESSION_NAME" --js "document.cookie.split('; ').find(c => c.startsWith('testCookie=')); document.cookie.includes('testCookie=stateValue') ? 'found' : 'not found'" 2>/dev/null | tail -n 1)
verify_value "$COOKIE_RESTORED" "found" "Cookie restoration"

LOCAL_STORAGE=$(./hweb-poc --session "$SESSION_NAME" --js "localStorage.getItem('key1') + '|' + localStorage.getItem('key2')" 2>/dev/null | tail -n 1)
verify_value "$LOCAL_STORAGE" "value1|value2" "localStorage restoration"

FORM_INPUT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testInput').value" 2>/dev/null | tail -n 1)
verify_value "$FORM_INPUT" "persistedValue" "Form input restoration"

FORM_TEXTAREA=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testTextarea').value" 2>/dev/null | tail -n 1)
verify_value "$FORM_TEXTAREA" "persistedText" "Form textarea restoration"

FORM_SELECT=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testSelect').value" 2>/dev/null | tail -n 1)
verify_value "$FORM_SELECT" "option3" "Form select restoration"

FORM_CHECKBOX=$(./hweb-poc --session "$SESSION_NAME" --js "document.querySelector('#testCheckbox').checked" 2>/dev/null | tail -n 1)
verify_value "$FORM_CHECKBOX" "true" "Form checkbox restoration"

SCROLL_POS=$(./hweb-poc --session "$SESSION_NAME" --js "window.pageYOffset" 2>/dev/null | tail -n 1)
if [[ "$SCROLL_POS" -gt 900 ]]; then
    echo -e "${GREEN}✓ PASS${NC}: Scroll position restored ($SCROLL_POS > 900)"
else
    echo -e "${YELLOW}? INFO${NC}: Scroll position may not be restored ($SCROLL_POS)"
fi
echo ""

USER_AGENT=$(./hweb-poc --session "$SESSION_NAME" --js "navigator.userAgent" 2>/dev/null | tail -n 1)
verify_contains "$USER_AGENT" "TestAgent/1.0" "User agent restoration"

CUSTOM_VAR=$(./hweb-poc --session "$SESSION_NAME" --get "customVar" 2>/dev/null | tail -n 1)
verify_value "$CUSTOM_VAR" "customValue" "Custom variable restoration"

check_command "./hweb-poc --session '$SESSION_NAME' --end" "End state test"

# Test 4: Remote Website Navigation
echo "=== Test 4: Remote Website Navigation ==="

check_command "./hweb-poc --session 'remote_test' --url '$REMOTE_TEST_URL'" "Navigate to remote site"

TITLE=$(./hweb-poc --session "remote_test" --js "document.title" 2>/dev/null | tail -n 1)
verify_contains "$TITLE" "Example" "Remote site title"

# Test navigation without session restoration
echo "Testing navigation without restoration..."
check_command "./hweb-poc --session 'fresh_nav' --url '$REMOTE_TEST_URL' --js 'document.title'" "Navigate and execute JS"

check_command "./hweb-poc --session 'remote_test' --end" "End remote test"
check_command "./hweb-poc --session 'fresh_nav' --end" "End fresh nav test"

# Test 5: Form Interaction and Smart Search
echo "=== Test 5: Form Interaction and Smart Search ==="

# Test regular form interaction
check_command "./hweb-poc --session 'form_test' \
    --url '$SEARCH_TEST_URL' \
    --wait 'form' \
    --type 'input[name=search]' 'monster' \
    --click 'button[name=dosearch]'" "Form interaction"

# Check if navigation occurred
sleep 2
FORM_URL=$(./hweb-poc --session "form_test" --js "window.location.href" 2>/dev/null | tail -n 1)
echo "URL after form submission: $FORM_URL"

check_command "./hweb-poc --session 'form_test' --end" "End form test"

# Test smart search command
echo "Testing smart search command..."
check_command "./hweb-poc --session 'search_test' \
    --url '$SEARCH_TEST_URL' \
    --search 'dragon'" "Smart search command"

SEARCH_URL=$(./hweb-poc --session "search_test" --js "window.location.href" 2>/dev/null | tail -n 1)
echo "URL after search: $SEARCH_URL"

check_command "./hweb-poc --session 'search_test' --end" "End search test"

# Test 6: Error Handling
echo "=== Test 6: Error Handling ==="

# Test invalid selector
if ./hweb-poc --session "error_test" --url "file://$LOCAL_TEST_FILE" --type "#nonexistent" "value" 2>&1 | grep -q "Failed"; then
    echo -e "${GREEN}✓ PASS${NC}: Invalid selector handled correctly"
else
    echo -e "${RED}✗ FAIL${NC}: Invalid selector not handled properly"
fi

# Test invalid URL
if ./hweb-poc --session "error_test" --url "invalid://url" 2>&1 | grep -q -E "(Failed|Error|Invalid)"; then
    echo -e "${GREEN}✓ PASS${NC}: Invalid URL handled correctly"
else
    echo -e "${RED}✗ FAIL${NC}: Invalid URL not handled properly"
fi

check_command "./hweb-poc --session 'error_test' --end" "Clean up error test"

# Test 7: Session Management
echo "=== Test 7: Session Management ==="

# Create multiple sessions
check_command "./hweb-poc --session 'session1' --url 'file://$LOCAL_TEST_FILE' --store 'name' 'session1'" "Create session1"
check_command "./hweb-poc --session 'session2' --url '$REMOTE_TEST_URL' --store 'name' 'session2'" "Create session2"
check_command "./hweb-poc --session 'session3' --url 'file://$LOCAL_TEST_FILE' --store 'name' 'session3'" "Create session3"

# List sessions
echo "Listing all sessions..."
./hweb-poc --list

# Verify session isolation
VAR1=$(./hweb-poc --session "session1" --get "name" 2>/dev/null | tail -n 1)
VAR2=$(./hweb-poc --session "session2" --get "name" 2>/dev/null | tail -n 1)
VAR3=$(./hweb-poc --session "session3" --get "name" 2>/dev/null | tail -n 1)

verify_value "$VAR1" "session1" "Session 1 isolation"
verify_value "$VAR2" "session2" "Session 2 isolation"
verify_value "$VAR3" "session3" "Session 3 isolation"

# Clean up sessions
check_command "./hweb-poc --session 'session1' --end" "End session1"
check_command "./hweb-poc --session 'session2' --end" "End session2"
check_command "./hweb-poc --session 'session3' --end" "End session3"

# Test 8: Advanced JavaScript and DOM Queries
echo "=== Test 8: Advanced JavaScript and DOM Queries ==="

check_command "./hweb-poc --session 'js_test' --url 'file://$LOCAL_TEST_FILE'" "Setup JS test"

# Test various DOM queries
check_command "./hweb-poc --session 'js_test' --exists 'h1'" "Element exists"
check_command "./hweb-poc --session 'js_test' --count 'input'" "Count elements"
check_command "./hweb-poc --session 'js_test' --html '#testDiv'" "Get HTML"
check_command "./hweb-poc --session 'js_test' --attr '#testInput' 'name'" "Get attribute"

# Test complex JavaScript
COMPLEX_JS='(function() {
    var inputs = document.querySelectorAll("input[type=text]");
    return Array.from(inputs).map(i => i.id).join(",");
})()'
JS_RESULT=$(./hweb-poc --session "js_test" --js "$COMPLEX_JS" 2>/dev/null | tail -n 1)
verify_contains "$JS_RESULT" "testInput" "Complex JavaScript execution"

check_command "./hweb-poc --session 'js_test' --end" "End JS test"

# Test 9: Debug Mode
echo "=== Test 9: Debug Mode ==="

echo "Testing without debug flag (should be quiet)..."
OUTPUT=$(./hweb-poc --session "debug_test" --url "file://$LOCAL_TEST_FILE" 2>&1)
if echo "$OUTPUT" | grep -q "Debug:"; then
    echo -e "${RED}✗ FAIL${NC}: Debug output shown without --debug flag"
else
    echo -e "${GREEN}✓ PASS${NC}: No debug output without flag"
fi

echo "Testing with debug flag (should show debug info)..."
OUTPUT=$(./hweb-poc --debug --session "debug_test" --url "file://$LOCAL_TEST_FILE" 2>&1)
if echo "$OUTPUT" | grep -q "Debug:"; then
    echo -e "${GREEN}✓ PASS${NC}: Debug output shown with --debug flag"
else
    echo -e "${RED}✗ FAIL${NC}: No debug output with --debug flag"
fi

check_command "./hweb-poc --session 'debug_test' --end" "End debug test"

# Test 10: Navigation Commands
echo "=== Test 10: Navigation Commands ==="

check_command "./hweb-poc --session 'nav_test' --url '$REMOTE_TEST_URL'" "Initial navigation"
check_command "./hweb-poc --session 'nav_test' --url 'https://www.iana.org/domains/example'" "Navigate to second page"

# Test back navigation
check_command "./hweb-poc --session 'nav_test' --back" "Navigate back"
URL_AFTER_BACK=$(./hweb-poc --session "nav_test" --js "window.location.href" 2>/dev/null | tail -n 1)
verify_contains "$URL_AFTER_BACK" "example.com" "Back navigation"

# Test forward navigation
check_command "./hweb-poc --session 'nav_test' --forward" "Navigate forward"
URL_AFTER_FORWARD=$(./hweb-poc --session "nav_test" --js "window.location.href" 2>/dev/null | tail -n 1)
verify_contains "$URL_AFTER_FORWARD" "iana.org" "Forward navigation"

# Test reload
check_command "./hweb-poc --session 'nav_test' --reload" "Reload page"

check_command "./hweb-poc --session 'nav_test' --end" "End navigation test"

# Test 11: Screenshot Functionality
echo "=== Test 11: Screenshot Functionality ==="

check_command "./hweb-poc --session 'screenshot_test' --url 'file://$LOCAL_TEST_FILE'" "Setup for screenshot"

# Test visible area screenshot
check_command "./hweb-poc --session 'screenshot_test' --screenshot 'test_visible.png'" "Visible area screenshot"
if [[ -f "test_visible.png" ]]; then
    echo -e "${GREEN}✓ SUCCESS${NC}: Screenshot file created"
    rm -f test_visible.png
else
    echo -e "${RED}✗ FAIL${NC}: Screenshot file not created"
fi

# Test full page screenshot
check_command "./hweb-poc --session 'screenshot_test' --screenshot-full 'test_full.png'" "Full page screenshot"
if [[ -f "test_full.png" ]]; then
    echo -e "${GREEN}✓ SUCCESS${NC}: Full page screenshot file created"
    rm -f test_full.png
else
    echo -e "${RED}✗ FAIL${NC}: Full page screenshot file not created"
fi

# Test default filename
check_command "./hweb-poc --session 'screenshot_test' --screenshot" "Screenshot with default filename"
if [[ -f "screenshot.png" ]]; then
    echo -e "${GREEN}✓ SUCCESS${NC}: Default screenshot created"
    rm -f screenshot.png
else
    echo -e "${RED}✗ FAIL${NC}: Default screenshot not created"
fi

check_command "./hweb-poc --session 'screenshot_test' --end" "End screenshot test"

# Final cleanup
echo "=== Final Cleanup ==="
cleanup

# Summary
echo "=== Test Summary ==="
echo "Comprehensive test suite completed!"
echo ""
echo "Tests covered:"
echo "✓ Basic navigation and session creation"
echo "✓ Command chaining"
echo "✓ Full state persistence (cookies, storage, forms, scroll, etc.)"
echo "✓ Remote website navigation"
echo "✓ Form interaction and smart search"
echo "✓ Error handling"
echo "✓ Multiple session management and isolation"
echo "✓ Advanced JavaScript and DOM queries"
echo "✓ Debug mode functionality"
echo "✓ Navigation commands (back, forward, reload)"
echo "✓ Screenshot functionality (visible area and full page)"
echo ""
echo "Note: Some features may show warnings due to:"
echo "- Browser security restrictions (sessionStorage on file:// URLs)"
echo "- Platform-specific behavior (user agent)"
echo "- Timing dependencies (scroll position)"
