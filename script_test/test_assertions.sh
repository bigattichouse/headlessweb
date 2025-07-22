#!/bin/bash

# Assertion System Test
# Tests all assertion functionality including exists, text, count, and JavaScript assertions

set -e

# Helper functions
source "$(dirname "$0")/test_helpers.sh"

# Configuration
TEST_FILE="/tmp/hweb_assertion_test.html"

# Create comprehensive test HTML
create_assertion_test_html() {
    cat > "$TEST_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Assertion Test Page</title>
    <script>
        window.testReady = false;
        window.testData = {
            count: 5,
            status: "ready", 
            isActive: true
        };
        
        setTimeout(function() {
            window.testReady = true;
            document.getElementById('dynamic-content').innerText = 'Dynamic content loaded';
        }, 100);
        
        window.complexFunction = function() {
            return document.querySelectorAll('.item').length > 3;
        };
    </script>
</head>
<body>
    <h1>Assertion Test Page</h1>
    <div class="container">
        <p class="status">System Ready</p>
        <div id="dynamic-content">Loading...</div>
        
        <!-- Test elements for counting -->
        <div class="item">Item 1</div>
        <div class="item">Item 2</div>
        <div class="item">Item 3</div>
        <div class="item">Item 4</div>
        <div class="item">Item 5</div>
        
        <!-- Form elements -->
        <form id="test-form">
            <input type="text" id="username" value="testuser">
            <input type="email" id="email" value="">
            <select id="country">
                <option value="us">United States</option>
                <option value="ca" selected>Canada</option>
                <option value="mx">Mexico</option>
            </select>
            <button type="submit">Submit</button>
        </form>
        
        <!-- Elements for various tests -->
        <div class="success-message" style="display: none;">Success!</div>
        <div class="error-message" style="display: none;">Error occurred</div>
        <div class="empty-div"></div>
        
        <!-- Data attributes -->
        <div data-value="42" data-status="active">Data element</div>
    </div>
</body>
</html>
EOF
    echo "Created assertion test HTML: $TEST_FILE"
}

# Test basic element existence assertions
test_element_existence() {
    echo "=== Test: Element Existence Assertions ==="
    
    create_assertion_test_html
    
    # Setup session
    $HWEB_EXECUTABLE --session 'assertion-test' --url "file://$TEST_FILE" >/dev/null 2>&1
    
    # Test element exists (should pass)
    run_test "Element Exists - Pass" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-exists 'h1'" \
        0 \
        "Existing element assertion"
    
    # Test element exists with explicit true
    run_test "Element Exists - Explicit True" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-exists '.container' true" \
        0 \
        "Existing element with explicit true"
    
    # Test element does not exist (should pass)
    run_test "Element Not Exists - Pass" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-exists '.nonexistent' false" \
        0 \
        "Non-existing element assertion"
    
    # Test element exists but expect false (should fail)
    run_test "Element Exists - Expect False (Fail)" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-exists 'h1' false" \
        1 \
        "Existing element expected to not exist"
    
    echo ""
}

# Test text content assertions
test_text_assertions() {
    echo "=== Test: Text Content Assertions ==="
    
    # Test exact text match (should pass)
    run_test "Text Match - Exact" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-text 'h1' 'Assertion Test Page'" \
        0 \
        "Exact text match"
    
    # Test text contains (should pass) 
    run_test "Text Match - Contains" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-text '.status' 'contains:Ready'" \
        0 \
        "Text contains match"
    
    # Test text mismatch (should fail)
    run_test "Text Match - Fail" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-text 'h1' 'Wrong Title'" \
        1 \
        "Text content mismatch"
    
    # Test not equals (should pass)
    run_test "Text Match - Not Equals" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-text '.status' '!=Wrong Text'" \
        0 \
        "Text not equals match"
    
    echo ""
}

# Test element count assertions
test_count_assertions() {
    echo "=== Test: Element Count Assertions ==="
    
    # Test exact count (should pass)
    run_test "Count - Exact Match" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-count '.item' '5'" \
        0 \
        "Exact element count"
    
    # Test greater than (should pass)
    run_test "Count - Greater Than" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-count '.item' '>3'" \
        0 \
        "Element count greater than"
    
    # Test less than (should pass)
    run_test "Count - Less Than" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-count '.item' '<10'" \
        0 \
        "Element count less than"
    
    # Test equals zero (should pass)
    run_test "Count - Zero Elements" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-count '.nonexistent' '==0'" \
        0 \
        "Zero element count"
    
    # Test count failure (should fail)
    run_test "Count - Fail" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-count '.item' '10'" \
        1 \
        "Incorrect element count"
    
    echo ""
}

# Test JavaScript assertions
test_javascript_assertions() {
    echo "=== Test: JavaScript Assertions ==="
    
    # Test simple JavaScript boolean (should pass)
    run_test "JS - Simple Boolean True" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-js 'true'" \
        0 \
        "Simple JavaScript true"
    
    # Test JavaScript boolean false (should pass)
    run_test "JS - Simple Boolean False" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-js 'false' false" \
        0 \
        "Simple JavaScript false"
    
    # Test JavaScript expression (should pass)
    run_test "JS - Expression True" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-js 'document.querySelectorAll(\".item\").length === 5'" \
        0 \
        "JavaScript expression evaluation"
    
    # Test complex JavaScript (should pass)
    run_test "JS - Complex Expression" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-js 'window.testData && window.testData.count > 0'" \
        0 \
        "Complex JavaScript condition"
    
    # Test JavaScript failure (should fail)
    run_test "JS - Expression False" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-js 'document.querySelectorAll(\".item\").length === 10'" \
        1 \
        "JavaScript expression that evaluates to false"
    
    echo ""
}

# Test JSON output mode
test_json_output() {
    echo "=== Test: JSON Output Mode ==="
    
    # Test JSON output for passing assertion
    local output=$($HWEB_EXECUTABLE --session 'assertion-test' --json --assert-exists 'h1' 2>/dev/null)
    if echo "$output" | jq -e '.result == "PASS"' >/dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: JSON output for passing assertion"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: JSON output for passing assertion"
        echo "  Output: $output"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    TESTS_RUN=$((TESTS_RUN + 1))
    
    echo ""
}

# Test silent mode
test_silent_mode() {
    echo "=== Test: Silent Mode ==="
    
    # Test silent mode with passing assertion (should produce no output)
    run_test_with_output "Silent - Pass" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --silent --assert-exists 'h1'" \
        "" \
        "Silent mode with passing assertion"
    
    # Test silent mode with failing assertion (should produce no output but exit 1)
    run_test "Silent - Fail" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --silent --assert-exists '.nonexistent'" \
        1 \
        "Silent mode with failing assertion"
    
    echo ""
}

# Test custom messages and timeouts
test_custom_features() {
    echo "=== Test: Custom Messages and Timeouts ==="
    
    # Test custom message
    run_test_with_output "Custom Message" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-exists 'h1' --message 'Page title should exist'" \
        "Page title should exist" \
        "Custom assertion message"
    
    # Test timeout (with element that appears dynamically)
    run_test "Timeout - Dynamic Content" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-text '#dynamic-content' 'Dynamic content loaded' --timeout 2000" \
        0 \
        "Assertion with timeout for dynamic content"
    
    echo ""
}

# Test error handling
test_error_handling() {
    echo "=== Test: Error Handling ==="
    
    # Test invalid selector syntax
    run_test "Error - Invalid Selector" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-exists 'invalid[selector'" \
        2 \
        "Invalid CSS selector handling"
    
    # Test timeout exceeded
    run_test "Error - Timeout" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-exists '.never-appears' --timeout 100" \
        1 \
        "Assertion timeout handling"
    
    echo ""
}

# Test edge cases
test_edge_cases() {
    echo "=== Test: Edge Cases ==="
    
    # Test empty text assertion
    run_test "Edge - Empty Text" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-text '.empty-div' ''" \
        0 \
        "Empty text content assertion"
    
    # Test very large numbers
    run_test "Edge - Large Number" \
        "$HWEB_EXECUTABLE --session 'assertion-test' --assert-js '1000000 > 999999'" \
        0 \
        "Large number comparison"
    
    echo ""
}

# Cleanup function
cleanup_assertions() {
    echo "=== Assertion Cleanup ==="
    $HWEB_EXECUTABLE --session 'assertion-test' --end >/dev/null 2>&1 || true
    rm -f "$TEST_FILE"
    echo "Assertion test files cleaned up"
    echo ""
}

# Main test execution
main() {
    echo "=== Assertion System Test Suite ==="
    echo ""
    
    test_element_existence
    test_text_assertions
    test_count_assertions
    test_javascript_assertions
    test_json_output
    test_silent_mode
    test_custom_features
    test_error_handling
    test_edge_cases
    
    cleanup_assertions
    
    print_test_summary "Assertion System"
    
    echo ""
    echo "Assertion Features Tested:"
    echo "✓ Basic element existence assertions"
    echo "✓ Text content matching (exact, contains, not-equals)"
    echo "✓ Element counting with comparison operators" 
    echo "✓ JavaScript expression evaluation"
    echo "✓ JSON output mode for tool integration"
    echo "✓ Silent mode for scripts and monitoring"
    echo "✓ Custom messages and timeout configuration"
    echo "✓ Error handling and edge cases"
}

# Run tests if called directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi