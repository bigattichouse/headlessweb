#!/bin/bash

set -e

echo "=== HeadlessWeb Assertion System Test Suite ==="
echo "Testing all assertion functionality of hweb"
echo ""

# Configuration
SESSION_DIR="$HOME/.hweb/sessions"
LOCAL_TEST_FILE="/tmp/hweb_assertion_test.html"
REMOTE_TEST_URL="https://example.com"

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
run_test() {
    local test_name="$1"
    local cmd="$2"
    local expected_exit_code="${3:-0}"
    local description="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${BLUE}Test $TESTS_RUN${NC}: $test_name"
    echo "Command: $cmd"
    
    set +e
    eval "$cmd" >/dev/null 2>&1
    local actual_exit_code=$?
    set -e
    
    if [[ $actual_exit_code -eq $expected_exit_code ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description (exit code: $actual_exit_code)"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected exit code: $expected_exit_code"
        echo "  Actual exit code: $actual_exit_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    echo ""
}

run_test_with_output() {
    local test_name="$1"
    local cmd="$2"
    local expected_output="$3"
    local description="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${BLUE}Test $TESTS_RUN${NC}: $test_name"
    echo "Command: $cmd"
    
    set +e
    local actual_output=$(eval "$cmd" 2>/dev/null)
    local exit_code=$?
    set -e
    
    if [[ "$actual_output" == *"$expected_output"* ]] && [[ $exit_code -eq 0 ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description"
        echo "  Output contains: '$expected_output'"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected to contain: '$expected_output'"
        echo "  Actual output: '$actual_output'"
        echo "  Exit code: $exit_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    echo ""
}

run_json_test() {
    local test_name="$1"
    local cmd="$2"
    local expected_result="$3"
    local description="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${BLUE}Test $TESTS_RUN${NC}: $test_name"
    echo "Command: $cmd"
    
    set +e
    local output=$(eval "$cmd" 2>/dev/null)
    local exit_code=$?
    set -e
    
    # Check if output is valid JSON and contains expected result
    if echo "$output" | jq -e ".result == \"$expected_result\"" >/dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: $description"
        echo "  JSON result: $expected_result"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected JSON result: '$expected_result'"
        echo "  Actual output: '$output'"
        echo "  Exit code: $exit_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
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
    rm -f test-results.xml
    rm -f *.png
    mkdir -p "$SESSION_DIR"
    echo "Cleaned up session directory and test files"
    echo ""
}

# Create comprehensive test HTML file
create_test_html() {
    cat > "$LOCAL_TEST_FILE" << 'EOF'
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
    echo "Created assertion test HTML file: $LOCAL_TEST_FILE"
}

# Start with cleanup
cleanup
create_test_html

echo "=== Test 1: Basic Element Existence Assertions ==="

# Setup session for testing
./hweb --session 'assertion-test' --url "file://$LOCAL_TEST_FILE" >/dev/null 2>&1

# Test element exists (should pass)
run_test "Element Exists - Pass" \
    "./hweb --session 'assertion-test' --assert-exists 'h1'" \
    0 \
    "Existing element assertion"

# Test element exists with explicit true
run_test "Element Exists - Explicit True" \
    "./hweb --session 'assertion-test' --assert-exists '.container' true" \
    0 \
    "Existing element with explicit true"

# Test element does not exist (should pass)
run_test "Element Not Exists - Pass" \
    "./hweb --session 'assertion-test' --assert-exists '.nonexistent' false" \
    0 \
    "Non-existing element assertion"

# Test element exists but expect false (should fail)
run_test "Element Exists - Expect False (Fail)" \
    "./hweb --session 'assertion-test' --assert-exists 'h1' false" \
    1 \
    "Existing element expected to not exist"

# Test element doesn't exist but expect true (should fail)
run_test "Element Not Exists - Expect True (Fail)" \
    "./hweb --session 'assertion-test' --assert-exists '.definitely-not-there'" \
    1 \
    "Non-existing element expected to exist"

echo "=== Test 2: Text Content Assertions ==="

# Test exact text match (should pass)
run_test "Text Match - Exact" \
    "./hweb --session 'assertion-test' --assert-text 'h1' 'Assertion Test Page'" \
    0 \
    "Exact text match"

# Test text contains (should pass)
run_test "Text Match - Contains" \
    "./hweb --session 'assertion-test' --assert-text '.status' 'contains:Ready'" \
    0 \
    "Text contains match"

# Test text mismatch (should fail)
run_test "Text Match - Fail" \
    "./hweb --session 'assertion-test' --assert-text 'h1' 'Wrong Title'" \
    1 \
    "Text content mismatch"

# Test regex pattern (should pass)
run_test "Text Match - Regex" \
    "./hweb --session 'assertion-test' --assert-text 'h1' '~=Test.*Page'" \
    0 \
    "Regex pattern match"

# Test not equals (should pass)
run_test "Text Match - Not Equals" \
    "./hweb --session 'assertion-test' --assert-text '.status' '!=Wrong Text'" \
    0 \
    "Text not equals match"

echo "=== Test 3: Element Count Assertions ==="

# Test exact count (should pass)
run_test "Count - Exact Match" \
    "./hweb --session 'assertion-test' --assert-count '.item' '5'" \
    0 \
    "Exact element count"

# Test greater than (should pass)
run_test "Count - Greater Than" \
    "./hweb --session 'assertion-test' --assert-count '.item' '>3'" \
    0 \
    "Element count greater than"

# Test less than (should pass)
run_test "Count - Less Than" \
    "./hweb --session 'assertion-test' --assert-count '.item' '<10'" \
    0 \
    "Element count less than"

# Test greater than or equal (should pass)
run_test "Count - Greater Equal" \
    "./hweb --session 'assertion-test' --assert-count '.item' '>=5'" \
    0 \
    "Element count greater than or equal"

# Test less than or equal (should pass)
run_test "Count - Less Equal" \
    "./hweb --session 'assertion-test' --assert-count '.item' '<=5'" \
    0 \
    "Element count less than or equal"

# Test not equals (should pass)
run_test "Count - Not Equals" \
    "./hweb --session 'assertion-test' --assert-count '.item' '!=3'" \
    0 \
    "Element count not equals"

# Test equals zero (should pass)
run_test "Count - Zero Elements" \
    "./hweb --session 'assertion-test' --assert-count '.nonexistent' '==0'" \
    0 \
    "Zero element count"

# Test count failure (should fail)
run_test "Count - Fail" \
    "./hweb --session 'assertion-test' --assert-count '.item' '10'" \
    1 \
    "Incorrect element count"

echo "=== Test 4: JavaScript Assertions ==="

# Test simple JavaScript boolean (should pass)
run_test "JS - Simple Boolean True" \
    "./hweb --session 'assertion-test' --assert-js 'true'" \
    0 \
    "Simple JavaScript true"

# Test JavaScript boolean false (should pass)
run_test "JS - Simple Boolean False" \
    "./hweb --session 'assertion-test' --assert-js 'false' false" \
    0 \
    "Simple JavaScript false"

# Test JavaScript expression (should pass)
run_test "JS - Expression True" \
    "./hweb --session 'assertion-test' --assert-js 'document.querySelectorAll(\".item\").length === 5'" \
    0 \
    "JavaScript expression evaluation"

# Test complex JavaScript (should pass)
run_test "JS - Complex Expression" \
    "./hweb --session 'assertion-test' --assert-js 'window.testData && window.testData.count > 0'" \
    0 \
    "Complex JavaScript condition"

# Test JavaScript failure (should fail)
run_test "JS - Expression False" \
    "./hweb --session 'assertion-test' --assert-js 'document.querySelectorAll(\".item\").length === 10'" \
    1 \
    "JavaScript expression that evaluates to false"

# Test JavaScript with return value (should pass)
run_test "JS - Return Value" \
    "./hweb --session 'assertion-test' --assert-js 'window.complexFunction()'" \
    0 \
    "JavaScript function call"

echo "=== Test 5: JSON Output Mode ==="

# Test JSON output for passing assertion
run_json_test "JSON - Pass" \
    "./hweb --session 'assertion-test' --json --assert-exists 'h1'" \
    "PASS" \
    "JSON output for passing assertion"

# Test JSON output for failing assertion
run_json_test "JSON - Fail" \
    "./hweb --session 'assertion-test' --json --assert-exists '.nonexistent'" \
    "FAIL" \
    "JSON output for failing assertion"

# Test JSON output for count assertion
run_json_test "JSON - Count" \
    "./hweb --session 'assertion-test' --json --assert-count '.item' '5'" \
    "PASS" \
    "JSON output for count assertion"

echo "=== Test 6: Silent Mode ==="

# Test silent mode with passing assertion (should produce no output)
run_test_with_output "Silent - Pass" \
    "./hweb --session 'assertion-test' --silent --assert-exists 'h1'" \
    "" \
    "Silent mode with passing assertion"

# Test silent mode with failing assertion (should produce no output but exit 1)
run_test "Silent - Fail" \
    "./hweb --session 'assertion-test' --silent --assert-exists '.nonexistent'" \
    1 \
    "Silent mode with failing assertion"

echo "=== Test 7: Custom Messages and Timeouts ==="

# Test custom message
run_test_with_output "Custom Message" \
    "./hweb --session 'assertion-test' --assert-exists 'h1' --message 'Page title should exist'" \
    "Page title should exist" \
    "Custom assertion message"

# Test timeout (with element that appears dynamically)
run_test "Timeout - Dynamic Content" \
    "./hweb --session 'assertion-test' --assert-text '#dynamic-content' 'Dynamic content loaded' --timeout 2000" \
    0 \
    "Assertion with timeout for dynamic content"

echo "=== Test 8: Test Suite Management ==="

# Test basic test suite
info_message "Running test suite with multiple assertions..."

# Start test suite and run multiple assertions
run_test "Test Suite - Basic" \
    "./hweb --test-suite start 'Basic Assertions' \
     --session 'suite-test' --url 'file://$LOCAL_TEST_FILE' \
     --assert-exists 'h1' \
     --assert-text '.status' 'System Ready' \
     --assert-count '.item' '5' \
     --test-suite end" \
    0 \
    "Basic test suite with multiple passing assertions"

# Test suite with failures
run_test "Test Suite - With Failures" \
    "./hweb --test-suite start 'Mixed Results' \
     --session 'suite-fail-test' --url 'file://$LOCAL_TEST_FILE' \
     --assert-exists 'h1' \
     --assert-exists '.nonexistent' \
     --assert-text '.status' 'Wrong Text' \
     --test-suite end" \
    1 \
    "Test suite with some failing assertions"

# Test JSON suite output
run_test_with_output "Test Suite - JSON" \
    "./hweb --test-suite start 'JSON Suite' --json \
     --session 'json-suite-test' --url 'file://$LOCAL_TEST_FILE' \
     --assert-exists 'h1' \
     --assert-count '.item' '5' \
     --test-suite end json" \
    '"suite": "JSON Suite"' \
    "Test suite with JSON output"

echo "=== Test 9: Complex Real-world Scenarios ==="

# Navigate to remote site for real-world testing
info_message "Testing with remote website..."
./hweb --session 'remote-test' --url "$REMOTE_TEST_URL" >/dev/null 2>&1

# Test remote site assertions
run_test "Remote - Title" \
    "./hweb --session 'remote-test' --assert-text 'title' 'contains:Example'" \
    0 \
    "Remote site title assertion"

run_test "Remote - Domain" \
    "./hweb --session 'remote-test' --assert-js 'window.location.hostname === \"example.com\"'" \
    0 \
    "Remote site domain check"

# Test form state assertions
./hweb --session 'assertion-test' --type '#username' 'newuser' --select '#country' 'us' >/dev/null 2>&1

run_test "Form State - Input" \
    "./hweb --session 'assertion-test' --assert-js 'document.getElementById(\"username\").value === \"newuser\"'" \
    0 \
    "Form input state assertion"

run_test "Form State - Select" \
    "./hweb --session 'assertion-test' --assert-js 'document.getElementById(\"country\").value === \"us\"'" \
    0 \
    "Form select state assertion"

echo "=== Test 10: Error Handling ==="

# Test invalid selector syntax
run_test "Error - Invalid Selector" \
    "./hweb --session 'assertion-test' --assert-exists 'invalid[selector'" \
    2 \
    "Invalid CSS selector handling"

# Test timeout exceeded
run_test "Error - Timeout" \
    "./hweb --session 'assertion-test' --assert-exists '.never-appears' --timeout 100" \
    1 \
    "Assertion timeout handling"

# Test invalid JavaScript
run_test "Error - Invalid JS" \
    "./hweb --session 'assertion-test' --assert-js 'this.is.invalid.javascript'" \
    2 \
    "Invalid JavaScript handling"

echo "=== Test 11: Edge Cases ==="

# Test empty text assertion
run_test "Edge - Empty Text" \
    "./hweb --session 'assertion-test' --assert-text '.empty-div' ''" \
    0 \
    "Empty text content assertion"

# Test element with special characters
./hweb --session 'assertion-test' --js 'document.querySelector("h1").setAttribute("data-special", "test@#$%")' >/dev/null 2>&1

run_test "Edge - Special Characters" \
    "./hweb --session 'assertion-test' --assert-js 'document.querySelector(\"h1\").getAttribute(\"data-special\") === \"test@#\$%\"'" \
    0 \
    "Special characters in assertion"

# Test very large numbers
run_test "Edge - Large Number" \
    "./hweb --session 'assertion-test' --assert-js '1000000 > 999999'" \
    0 \
    "Large number comparison"

echo "=== Test 12: Performance and Reliability ==="

# Test multiple rapid assertions
info_message "Testing assertion performance with rapid execution..."

start_time=$(date +%s%N)
for i in {1..10}; do
    ./hweb --session 'assertion-test' --silent --assert-exists 'h1' || true
done
end_time=$(date +%s%N)
duration=$(( (end_time - start_time) / 1000000 )) # Convert to milliseconds

if [[ $duration -lt 5000 ]]; then # Less than 5 seconds for 10 assertions
    echo -e "${GREEN}✓ PASS${NC}: Performance test - 10 assertions in ${duration}ms"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ FAIL${NC}: Performance test - 10 assertions took ${duration}ms (too slow)"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
TESTS_RUN=$((TESTS_RUN + 1))
echo ""

# Test assertion consistency (same assertion multiple times should give same result)
result1=$(./hweb --session 'assertion-test' --silent --assert-count '.item' '5'; echo $?)
result2=$(./hweb --session 'assertion-test' --silent --assert-count '.item' '5'; echo $?)
result3=$(./hweb --session 'assertion-test' --silent --assert-count '.item' '5'; echo $?)

if [[ $result1 -eq $result2 && $result2 -eq $result3 ]]; then
    echo -e "${GREEN}✓ PASS${NC}: Assertion consistency test"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ FAIL${NC}: Assertion consistency test - results varied: $result1, $result2, $result3"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
TESTS_RUN=$((TESTS_RUN + 1))
echo ""

# Cleanup sessions
./hweb --session 'assertion-test' --end >/dev/null 2>&1
./hweb --session 'remote-test' --end >/dev/null 2>&1
./hweb --session 'suite-test' --end >/dev/null 2>&1
./hweb --session 'suite-fail-test' --end >/dev/null 2>&1
./hweb --session 'json-suite-test' --end >/dev/null 2>&1

# Final cleanup
cleanup

# Test Summary
echo "=== Assertion System Test Summary ==="
echo -e "${BLUE}Total Tests Run: $TESTS_RUN${NC}"
echo -e "${GREEN}Tests Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Tests Failed: $TESTS_FAILED${NC}"

if [[ $TESTS_FAILED -eq 0 ]]; then
    echo -e "${GREEN}🎉 ALL ASSERTION TESTS PASSED!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some assertion tests failed${NC}"
    echo "Success rate: $(( TESTS_PASSED * 100 / TESTS_RUN ))%"
    exit 1
fi

echo ""
echo "Assertion Features Tested:"
echo "✓ Basic element existence assertions"
echo "✓ Text content matching (exact, contains, regex, not-equals)"
echo "✓ Element counting with comparison operators"
echo "✓ JavaScript expression evaluation"
echo "✓ JSON output mode for tool integration"
echo "✓ Silent mode for scripts and monitoring"
echo "✓ Custom messages and timeout configuration"
echo "✓ Test suite management with multiple assertions"
echo "✓ Complex real-world scenarios"
echo "✓ Error handling and edge cases"
echo "✓ Performance and reliability testing"
echo ""
echo "The assertion system is ready for CI/CD integration!"
