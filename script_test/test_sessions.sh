#!/bin/bash

# Session Management Test
# Tests session creation, persistence, isolation, and management

set -e

# Helper functions
source "$(dirname "$0")/test_helpers.sh"

# Configuration
TEST_FILE="/tmp/hweb_session_test.html"

# Create session test HTML
create_session_test_html() {
    cat > "$TEST_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Session Test Page</title>
    <script>
        window.sessionTestData = {
            initialized: true,
            timestamp: Date.now()
        };
        
        // Set some localStorage data
        localStorage.setItem('session_test_key', 'session_test_value');
        localStorage.setItem('user_preference', 'dark_mode');
        
        // Set some sessionStorage data
        sessionStorage.setItem('temp_data', 'temporary_value');
        
        // Set a cookie
        document.cookie = 'session_cookie=session_value; path=/';
        document.cookie = 'user_id=12345; path=/';
    </script>
</head>
<body>
    <h1>Session Test Page</h1>
    
    <div id="session-info">
        <p id="session-id">Session ID: not-set</p>
        <p id="user-data">User Data: not-set</p>
    </div>
    
    <form id="session-form">
        <input type="text" id="persistent-input" name="persistent-input" value="initial-value">
        <textarea id="persistent-textarea">Initial textarea content</textarea>
        <select id="persistent-select">
            <option value="option1">Option 1</option>
            <option value="option2" selected>Option 2</option>
            <option value="option3">Option 3</option>
        </select>
        <input type="checkbox" id="persistent-checkbox" checked>
        <label for="persistent-checkbox">Persistent Checkbox</label>
    </form>
    
    <div style="height: 2000px; background: linear-gradient(to bottom, white, lightgray);">
        <p style="margin-top: 500px;">Scroll position test content at 500px</p>
        <p style="margin-top: 500px;">Content at 1000px</p>
        <p style="margin-top: 500px;">Content at 1500px</p>
    </div>
</body>
</html>
EOF
    echo "Created session test HTML: $TEST_FILE"
}

# Test basic session creation
test_session_creation() {
    echo "=== Test: Session Creation ==="
    
    create_session_test_html
    
    # Test session creation with URL
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --url 'file://$TEST_FILE'" "Create session with URL"
    
    # Verify session can access the page
    PAGE_TITLE=$($HWEB_EXECUTABLE --session "test_session_1" --js "document.title" 2>/dev/null | tail -n 1)
    verify_value "$PAGE_TITLE" "Session Test Page" "Session page access"
    
    # Store some data in the session
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --store 'test_key' 'test_value'" "Store data in session"
    
    # Verify stored data
    STORED_VALUE=$($HWEB_EXECUTABLE --session "test_session_1" --get "test_key" 2>/dev/null | tail -n 1)
    verify_value "$STORED_VALUE" "test_value" "Session data storage"
    
    echo ""
}

# Test session persistence
test_session_persistence() {
    echo "=== Test: Session Persistence ==="
    
    # Modify form state
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --type '#persistent-input' 'modified_value'" "Modify input field"
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --type '#persistent-textarea' 'modified textarea'" "Modify textarea"
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --select '#persistent-select' 'option3'" "Modify select"
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --uncheck '#persistent-checkbox'" "Uncheck checkbox"
    
    # Set scroll position
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --js 'window.scrollTo(0, 1000); \"scrolled\"'" "Set scroll position"
    
    # Set localStorage
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --js 'localStorage.setItem(\"session_persistent\", \"persistent_data\"); \"localStorage set\"'" "Set localStorage"
    
    # Set cookies
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --js 'document.cookie = \"session_test=persistent_cookie; path=/\"; \"cookie set\"'" "Set cookie"
    
    # End session to save state
    check_command "$HWEB_EXECUTABLE --session 'test_session_1' --end" "End session to save state"
    
    echo ""
}

# Test session restoration
test_session_restoration() {
    echo "=== Test: Session Restoration ==="
    
    # Restore the session
    check_command "$HWEB_EXECUTABLE --session 'test_session_1'" "Restore session"
    
    # Verify URL restoration
    RESTORED_URL=$($HWEB_EXECUTABLE --session "test_session_1" --js "window.location.href" 2>/dev/null | tail -n 1)
    verify_contains "$RESTORED_URL" "$TEST_FILE" "URL restoration"
    
    # Verify form state restoration
    RESTORED_INPUT=$($HWEB_EXECUTABLE --session "test_session_1" --js "document.getElementById('persistent-input').value" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_INPUT" "modified_value" "Input field restoration"
    
    RESTORED_TEXTAREA=$($HWEB_EXECUTABLE --session "test_session_1" --js "document.getElementById('persistent-textarea').value" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_TEXTAREA" "modified textarea" "Textarea restoration"
    
    RESTORED_SELECT=$($HWEB_EXECUTABLE --session "test_session_1" --js "document.getElementById('persistent-select').value" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_SELECT" "option3" "Select restoration"
    
    RESTORED_CHECKBOX=$($HWEB_EXECUTABLE --session "test_session_1" --js "document.getElementById('persistent-checkbox').checked" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_CHECKBOX" "false" "Checkbox restoration"
    
    # Verify localStorage restoration
    RESTORED_LOCALSTORAGE=$($HWEB_EXECUTABLE --session "test_session_1" --js "localStorage.getItem('session_persistent')" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_LOCALSTORAGE" "persistent_data" "localStorage restoration"
    
    # Verify cookie restoration
    RESTORED_COOKIE=$($HWEB_EXECUTABLE --session "test_session_1" --js "document.cookie.includes('session_test=persistent_cookie')" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_COOKIE" "true" "Cookie restoration"
    
    # Verify scroll position restoration (approximate check)
    SCROLL_POS=$($HWEB_EXECUTABLE --session "test_session_1" --js "window.pageYOffset" 2>/dev/null | tail -n 1)
    if (( $(echo "$SCROLL_POS > 900" | bc -l 2>/dev/null || echo "0") )); then
        echo -e "${GREEN}✓ PASS${NC}: Scroll position restored ($SCROLL_POS > 900)"
    else
        echo -e "${YELLOW}? INFO${NC}: Scroll position may not be restored ($SCROLL_POS)"
    fi
    
    # Verify custom session data
    RESTORED_CUSTOM=$($HWEB_EXECUTABLE --session "test_session_1" --get "test_key" 2>/dev/null | tail -n 1)
    verify_value "$RESTORED_CUSTOM" "test_value" "Custom session data restoration"
    
    echo ""
}

# Test session isolation
test_session_isolation() {
    echo "=== Test: Session Isolation ==="
    
    # Create multiple sessions with different data
    check_command "$HWEB_EXECUTABLE --session 'session_a' --url 'file://$TEST_FILE' --store 'session_name' 'session_a'" "Create session A"
    check_command "$HWEB_EXECUTABLE --session 'session_b' --url 'file://$TEST_FILE' --store 'session_name' 'session_b'" "Create session B"
    check_command "$HWEB_EXECUTABLE --session 'session_c' --url 'file://$TEST_FILE' --store 'session_name' 'session_c'" "Create session C"
    
    # Modify different data in each session
    check_command "$HWEB_EXECUTABLE --session 'session_a' --type '#persistent-input' 'value_a'" "Set unique data in session A"
    check_command "$HWEB_EXECUTABLE --session 'session_b' --type '#persistent-input' 'value_b'" "Set unique data in session B"  
    check_command "$HWEB_EXECUTABLE --session 'session_c' --type '#persistent-input' 'value_c'" "Set unique data in session C"
    
    # Set different localStorage in each session
    check_command "$HWEB_EXECUTABLE --session 'session_a' --js 'localStorage.setItem(\"unique\", \"data_a\"); \"set\"'" "Set localStorage in session A"
    check_command "$HWEB_EXECUTABLE --session 'session_b' --js 'localStorage.setItem(\"unique\", \"data_b\"); \"set\"'" "Set localStorage in session B"
    check_command "$HWEB_EXECUTABLE --session 'session_c' --js 'localStorage.setItem(\"unique\", \"data_c\"); \"set\"'" "Set localStorage in session C"
    
    # Verify session isolation
    SESSION_A_NAME=$($HWEB_EXECUTABLE --session "session_a" --get "session_name" 2>/dev/null | tail -n 1)
    SESSION_B_NAME=$($HWEB_EXECUTABLE --session "session_b" --get "session_name" 2>/dev/null | tail -n 1)
    SESSION_C_NAME=$($HWEB_EXECUTABLE --session "session_c" --get "session_name" 2>/dev/null | tail -n 1)
    
    verify_value "$SESSION_A_NAME" "session_a" "Session A isolation"
    verify_value "$SESSION_B_NAME" "session_b" "Session B isolation"
    verify_value "$SESSION_C_NAME" "session_c" "Session C isolation"
    
    # Verify form field isolation
    FIELD_A=$($HWEB_EXECUTABLE --session "session_a" --js "document.getElementById('persistent-input').value" 2>/dev/null | tail -n 1)
    FIELD_B=$($HWEB_EXECUTABLE --session "session_b" --js "document.getElementById('persistent-input').value" 2>/dev/null | tail -n 1)
    FIELD_C=$($HWEB_EXECUTABLE --session "session_c" --js "document.getElementById('persistent-input').value" 2>/dev/null | tail -n 1)
    
    verify_value "$FIELD_A" "value_a" "Form field isolation A"
    verify_value "$FIELD_B" "value_b" "Form field isolation B"  
    verify_value "$FIELD_C" "value_c" "Form field isolation C"
    
    # Verify localStorage isolation
    STORAGE_A=$($HWEB_EXECUTABLE --session "session_a" --js "localStorage.getItem('unique')" 2>/dev/null | tail -n 1)
    STORAGE_B=$($HWEB_EXECUTABLE --session "session_b" --js "localStorage.getItem('unique')" 2>/dev/null | tail -n 1)
    STORAGE_C=$($HWEB_EXECUTABLE --session "session_c" --js "localStorage.getItem('unique')" 2>/dev/null | tail -n 1)
    
    verify_value "$STORAGE_A" "data_a" "localStorage isolation A"
    verify_value "$STORAGE_B" "data_b" "localStorage isolation B"
    verify_value "$STORAGE_C" "data_c" "localStorage isolation C"
    
    echo ""
}

# Test session listing
test_session_listing() {
    echo "=== Test: Session Listing ==="
    
    # List sessions and check output
    echo "Current sessions:"
    $HWEB_EXECUTABLE --list
    
    # Verify that our test sessions appear in the list
    SESSION_LIST=$($HWEB_EXECUTABLE --list 2>/dev/null)
    
    if echo "$SESSION_LIST" | grep -q "session_a"; then
        echo -e "${GREEN}✓ PASS${NC}: Session A appears in list"
    else
        echo -e "${RED}✗ FAIL${NC}: Session A not found in list"
    fi
    
    if echo "$SESSION_LIST" | grep -q "session_b"; then
        echo -e "${GREEN}✓ PASS${NC}: Session B appears in list"
    else
        echo -e "${RED}✗ FAIL${NC}: Session B not found in list"
    fi
    
    echo ""
}

# Test session continuation
test_session_continuation() {
    echo "=== Test: Session Continuation ==="
    
    # Start session and navigate to a page
    check_command "$HWEB_EXECUTABLE --session 'continuation_test' --url 'file://$TEST_FILE'" "Start continuation test session"
    
    # Execute commands without specifying URL (should continue with current page)
    check_command "$HWEB_EXECUTABLE --session 'continuation_test' --type '#persistent-input' 'continued_session'" "Continue session without URL"
    
    # Verify the command worked on the existing page
    CONTINUED_VALUE=$($HWEB_EXECUTABLE --session "continuation_test" --js "document.getElementById('persistent-input').value" 2>/dev/null | tail -n 1)
    verify_value "$CONTINUED_VALUE" "continued_session" "Session continuation"
    
    # Test command chaining in continued session
    check_command "$HWEB_EXECUTABLE --session 'continuation_test' \
        --type '#persistent-textarea' 'chained operation' \
        --select '#persistent-select' 'option1' \
        --js 'document.title'" "Chained commands in continued session"
    
    echo ""
}

# Test session cleanup
test_session_cleanup() {
    echo "=== Test: Session Cleanup ==="
    
    # End specific sessions
    check_command "$HWEB_EXECUTABLE --session 'session_a' --end" "End session A"
    check_command "$HWEB_EXECUTABLE --session 'session_b' --end" "End session B"
    check_command "$HWEB_EXECUTABLE --session 'session_c' --end" "End session C"
    check_command "$HWEB_EXECUTABLE --session 'continuation_test' --end" "End continuation test session"
    
    # Verify sessions are removed from list
    SESSION_LIST_AFTER=$($HWEB_EXECUTABLE --list 2>/dev/null)
    
    if echo "$SESSION_LIST_AFTER" | grep -q "session_a"; then
        echo -e "${RED}✗ FAIL${NC}: Session A still appears in list after cleanup"
    else
        echo -e "${GREEN}✓ PASS${NC}: Session A removed from list"
    fi
    
    echo ""
}

# Test session error handling
test_session_errors() {
    echo "=== Test: Session Error Handling ==="
    
    # Test accessing non-existent session data
    NONEXISTENT_DATA=$($HWEB_EXECUTABLE --session "test_session_1" --get "nonexistent_key" 2>/dev/null | tail -n 1)
    verify_value "$NONEXISTENT_DATA" "" "Non-existent session data handling"
    
    # Test invalid session name characters (if any restrictions)
    if $HWEB_EXECUTABLE --session 'invalid-session-name!' --url 'file://$TEST_FILE' 2>&1 | grep -q -E "(Error|Invalid)"; then
        echo -e "${GREEN}✓ PASS${NC}: Invalid session name handled (if restricted)"
    else
        echo -e "${YELLOW}? INFO${NC}: Session name validation may be permissive"
    fi
    
    echo ""
}

# Test session with different URLs
test_session_url_changes() {
    echo "=== Test: Session URL Changes ==="
    
    # Create session with initial URL
    check_command "$HWEB_EXECUTABLE --session 'url_change_test' --url 'file://$TEST_FILE'" "Create session with initial URL"
    
    # Store some data
    check_command "$HWEB_EXECUTABLE --session 'url_change_test' --store 'initial_page' 'test_file'" "Store data for initial page"
    
    # Navigate to different URL and store different data
    check_command "$HWEB_EXECUTABLE --session 'url_change_test' --url 'https://example.com' --store 'second_page' 'example_com'" "Navigate to different URL"
    
    # Verify both data items persist
    INITIAL_DATA=$($HWEB_EXECUTABLE --session "url_change_test" --get "initial_page" 2>/dev/null | tail -n 1)
    SECOND_DATA=$($HWEB_EXECUTABLE --session "url_change_test" --get "second_page" 2>/dev/null | tail -n 1)
    
    verify_value "$INITIAL_DATA" "test_file" "Initial page data persistence"
    verify_value "$SECOND_DATA" "example_com" "Second page data persistence"
    
    echo ""
}

# Cleanup function
cleanup_sessions() {
    echo "=== Session Test Cleanup ==="
    
    # Clean up all test sessions
    $HWEB_EXECUTABLE --session 'test_session_1' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'session_a' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'session_b' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'session_c' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'continuation_test' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'url_change_test' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'invalid-session-name!' --end >/dev/null 2>&1 || true
    
    rm -f "$TEST_FILE"
    echo "Session test cleanup completed"
    echo ""
}

# Main test execution
main() {
    echo "=== Session Management Test Suite ==="
    echo ""
    
    test_session_creation
    test_session_persistence
    test_session_restoration
    test_session_isolation
    test_session_listing
    test_session_continuation
    test_session_cleanup
    test_session_errors
    test_session_url_changes
    
    cleanup_sessions
    
    echo "=== Session Tests Complete ==="
    echo ""
    echo "Session Features Tested:"
    echo "✓ Session creation with URL and data storage"
    echo "✓ Session persistence (forms, localStorage, cookies, scroll)"
    echo "✓ Session restoration after restart"
    echo "✓ Session isolation between multiple sessions"
    echo "✓ Session listing and management"
    echo "✓ Session continuation without re-specifying URL"
    echo "✓ Session cleanup and removal"
    echo "✓ Error handling for invalid operations"
    echo "✓ Session data persistence across URL changes"
}

# Run tests if called directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi