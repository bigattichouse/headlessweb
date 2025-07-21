#!/bin/bash

# Navigation Feature Test
# Tests basic navigation, session creation, and URL handling

set -e

# Configuration
LOCAL_TEST_FILE="/tmp/hweb_navigation_test.html"

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Helper functions
source "$(dirname "$0")/test_helpers.sh"

# Create test HTML file
create_navigation_test_html() {
    cat > "$LOCAL_TEST_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Navigation Test Page</title></head>
<body>
<h1>Navigation Test Content</h1>
<div id="testDiv">This is a test div.</div>
<a href="https://example.com" id="externalLink">External Link</a>
<a href="#section2" id="hashLink">Hash Link</a>
<div id="section2" style="margin-top: 1000px;">Section 2</div>
</body>
</html>
EOF
    echo "Created navigation test HTML file: $LOCAL_TEST_FILE"
}

# Test basic navigation
test_basic_navigation() {
    echo "=== Test: Basic Navigation ==="
    
    create_navigation_test_html
    
    check_command "./hweb --session 'nav_basic' --url 'file://$LOCAL_TEST_FILE'" "Navigate to local file"
    
    DOM_CHECK=$(./hweb --session "nav_basic" --text "#testDiv" 2>/dev/null | tail -n 1)
    verify_value "$DOM_CHECK" "This is a test div." "Local file DOM access"
    
    check_command "./hweb --session 'nav_basic' --end" "End basic navigation session"
    
    rm -f "$LOCAL_TEST_FILE"
    echo ""
}

# Test session creation and restoration
test_session_management() {
    echo "=== Test: Session Management ==="
    
    create_navigation_test_html
    
    # Create session with URL
    check_command "./hweb --session 'nav_session' --url 'file://$LOCAL_TEST_FILE'" "Create session with URL"
    
    # Store some state
    check_command "./hweb --session 'nav_session' --store 'test_var' 'test_value'" "Store session variable"
    
    # End session
    check_command "./hweb --session 'nav_session' --end" "End session"
    
    # Restore session
    check_command "./hweb --session 'nav_session'" "Restore session"
    
    # Verify state restoration
    RESTORED_VAR=$(./hweb --session "nav_session" --get "test_var" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_VAR" "test_value" "Session state restoration"
    
    # Verify URL restoration
    RESTORED_URL=$(./hweb --session "nav_session" --js "window.location.href" 2>/dev/null | tail -n 1)
    verify_contains "$RESTORED_URL" "$LOCAL_TEST_FILE" "Session URL restoration"
    
    check_command "./hweb --session 'nav_session' --end" "Clean up session"
    
    rm -f "$LOCAL_TEST_FILE"
    echo ""
}

# Test navigation commands
test_navigation_commands() {
    echo "=== Test: Navigation Commands (Back/Forward/Reload) ==="
    
    # Test with remote URLs for proper navigation history
    check_command "./hweb --session 'nav_commands' --url 'https://example.com'" "Navigate to first page"
    check_command "./hweb --session 'nav_commands' --url 'https://www.iana.org/domains/example'" "Navigate to second page"
    
    # Test back navigation
    check_command "./hweb --session 'nav_commands' --back" "Navigate back"
    URL_AFTER_BACK=$(./hweb --session "nav_commands" --js "window.location.href" 2>/dev/null | tail -n 1)
    verify_contains "$URL_AFTER_BACK" "example.com" "Back navigation"
    
    # Test forward navigation
    check_command "./hweb --session 'nav_commands' --forward" "Navigate forward"
    URL_AFTER_FORWARD=$(./hweb --session "nav_commands" --js "window.location.href" 2>/dev/null | tail -n 1)
    verify_contains "$URL_AFTER_FORWARD" "iana.org" "Forward navigation"
    
    # Test reload
    check_command "./hweb --session 'nav_commands' --reload" "Reload page"
    
    check_command "./hweb --session 'nav_commands' --end" "End navigation commands test"
    echo ""
}

# Test URL validation and error handling
test_url_validation() {
    echo "=== Test: URL Validation ==="
    
    # Test invalid URL
    if ./hweb --session "url_error" --url "invalid://url" 2>&1 | grep -q -E "(Failed|Error|Invalid)"; then
        echo -e "${GREEN}✓ PASS${NC}: Invalid URL handled correctly"
    else
        echo -e "${RED}✗ FAIL${NC}: Invalid URL not handled properly"
    fi
    
    # Test file URL security
    if ./hweb --session "url_error" --url "file:///etc/passwd" 2>&1 | grep -q -E "(Error|Invalid|unsafe)"; then
        echo -e "${GREEN}✓ PASS${NC}: File URL security check working"
    else
        echo -e "${RED}✗ FAIL${NC}: File URL security check not working"
    fi
    
    check_command "./hweb --session 'url_error' --end" "Clean up URL error test"
    echo ""
}

# Main test execution
main() {
    echo "=== Navigation Feature Test Suite ==="
    echo ""
    
    test_basic_navigation
    test_session_management  
    test_navigation_commands
    test_url_validation
    
    echo "=== Navigation Tests Complete ==="
}

# Run tests if called directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi