# HeadlessWeb Test Strategy

This document outlines the testing strategy for HeadlessWeb, including the assertion system, current test failures, and links to detailed test information.

## Assertion System

---

# HeadlessWeb Assertion System Examples

## Basic Assertions

### Element Existence
```bash
# Check if login form exists
./hweb --url "https://example.com/login" --assert-exists "#login-form"

# Check if error message does NOT exist
./hweb --session test --assert-exists ".error-message" false

# With custom message and timeout
./hweb --session test --assert-exists "#submit-button" \
  --message "Submit button should be visible" --timeout 10000
```

### Text Content Assertions
```bash
# Exact text match
./hweb --session test --assert-text "h1" "Welcome to Dashboard"

# Contains text
./hweb --session test --assert-text ".status" "contains:Success"

# Regex pattern matching
./hweb --session test --assert-text ".version" "~=v\d+\.\d+\.\d+"

# Case-insensitive comparison
./hweb --session test --assert-text ".message" "error" --case-insensitive
```

### Element Count Assertions
```bash
# Exact count
./hweb --session test --assert-count ".product-item" "5"

# Comparison operators
./hweb --session test --assert-count ".error" "==0"
./hweb --session test --assert-count ".notification" ">2"
./hweb --session test --assert-count ".item" "<=10"
./hweb --session test --assert-count ".warning" "!=1"
```

### JavaScript Assertions
```bash
# Simple boolean check
./hweb --session test --assert-js "window.appReady === true"

# Check if data is loaded
./hweb --session test --assert-js "document.querySelectorAll('.data-row').length > 0"

# Complex validation
./hweb --session test --assert-js "window.userData && window.userData.isLoggedIn"
```

## JSON Output Mode

### Single Assertions
```bash
# JSON output for tool integration
./hweb --session test --json --assert-exists ".dashboard"
# Output: {"assertion": "exists", "selector": ".dashboard", "result": "PASS", "expected": "true", "actual": "true", "duration_ms": 150}

./hweb --session test --json --assert-text ".status" "Complete" --silent
# Output: {"assertion": "text", "selector": ".status", "result": "PASS", "expected": "Complete", "actual": "Complete", "duration_ms": 75}
```

### Test Suites
```bash
# JSON suite output
./hweb --test-suite start "Login Tests" --json \
  --url "https://app.com/login" \
  --assert-exists "#username" \
  --assert-exists "#password" \
  --assert-text "h1" "Login" \
  --test-suite end json

# Output:
# {
#   "suite": "Login Tests",
#   "total": 3,
#   "passed": 3,
#   "failed": 0,
#   "errors": 0,
#   "duration_ms": 1250,
#   "tests": [
#     {"assertion": "exists", "selector": "#username", "result": "PASS", ...},
#     {"assertion": "exists", "selector": "#password", "result": "PASS", ...},
#     {"assertion": "text", "selector": "h1", "result": "PASS", ...}
#   ]
# }
```

## Test Suite Management

### Basic Test Suite
```bash
./hweb --test-suite start "User Registration Flow"
./hweb --session reg --url "https://app.com/register"
./hweb --session reg --assert-exists "#email" --message "Email field should be present"
./hweb --session reg --type "#email" "test@example.com"
./hweb --session reg --assert-text ".validation" "contains:valid"
./hweb --test-suite end
```

### JUnit XML Output for CI/CD
```bash
# Generate JUnit XML for Jenkins/GitHub Actions
./hweb --test-suite start "API Dashboard Tests" \
  --url "https://dashboard.api.com" \
  --assert-exists ".dashboard-container" \
  --assert-count ".metric-card" ">3" \
  --assert-text ".status-indicator" "Online" \
  --test-suite end junit > test-results.xml
```

### Complex Workflow Testing
```bash
# E2E checkout flow with assertions
./hweb --test-suite start "E2E Checkout Flow"

# Navigate and verify product page
./hweb --session checkout --url "https://store.com/product/123"
./hweb --session checkout --assert-exists ".product-details"
./hweb --session checkout --assert-text ".price" "contains:$"

# Add to cart
./hweb --session checkout --click "#add-to-cart"
./hweb --session checkout --assert-text ".cart-count" ">0"

# Navigate to checkout
./hweb --session checkout --click "#checkout-button"
./hweb --session checkout --assert-exists "#checkout-form"
./hweb --session checkout --assert-count ".required-field" ">=3"

# Fill form and verify
./hweb --session checkout --type "#email" "customer@example.com"
./hweb --session checkout --assert-js "document.getElementById('email').value.includes('@')"

./hweb --test-suite end
```

## CI/CD Integration Examples

### GitHub Actions
```yaml
name: Web UI Tests
on: [push, pull_request]

jobs:
  ui-tests:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Install HeadlessWeb
      run: |        # Install dependencies and build hweb
        sudo apt-get update
        sudo apt-get install -y libgtk-4-dev libwebkit2gtk-4.1-dev libjsoncpp-dev
        make
    
    - name: Run Login Tests
      run: |        ./hweb --test-suite start "Login Tests" \
          --json \
          --url "${{ secrets.TEST_URL }}/login" \
          --assert-exists "#login-form" \
          --type "#username" "${{ secrets.TEST_USER }}" \
          --type "#password" "${{ secrets.TEST_PASS }}" \
          --click "#login-button" \
          --wait-nav \
          --assert-exists ".dashboard" \
          --assert-text ".welcome" "contains:Welcome" \
          --test-suite end junit > login-test-results.xml
    
    - name: Publish Test Results
      uses: dorny/test-reporter@v1
      if: always()
      with:
        name: UI Test Results
        path: '*-test-results.xml'
        reporter: java-junit
```

### Jenkins Pipeline
```groovy
pipeline {
    agent any
    
    stages {
        stage('UI Tests') {
            steps {
                script {
                    // Run test suite with proper exit codes
                    def result = sh(
                        script: '''                            ./hweb --test-suite start "Production Health Check" \
                              --url "${PROD_URL}" \
                              --assert-exists ".health-status" \
                              --assert-text ".health-status" "Healthy" \
                              --assert-count ".error" "==0" \
                              --assert-js "window.performance.now() < 5000" \
                              --test-suite end junit
                        ''',
                        returnStatus: true
                    )
                    
                    if (result != 0) {
                        currentBuild.result = 'FAILURE'
                        error("UI tests failed with exit code ${result}")
                    }
                }
            }
            post {
                always {
                    // Publish JUnit results
                    junit '*-test-results.xml'
                }
            }
        }
    }
}
```

### Silent Mode for Scripts
```bash
#!/bin/bash
# health-check.sh - Silent monitoring script

# Check if dashboard is accessible (exit code only)
./hweb --silent --url "https://dashboard.internal.com" \
  --assert-exists ".dashboard" \
  --assert-text ".status" "Online" \
  --assert-count ".error" "==0"

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ Dashboard health check passed"
else
    echo "❌ Dashboard health check failed (exit code: $EXIT_CODE)"
    # Trigger alert
    curl -X POST "$SLACK_WEBHOOK" -d '{"text":"Dashboard health check failed!"}'
fi

exit $EXIT_CODE
```

## Advanced Assertion Patterns

### Conditional Assertions
```bash
# Check if login is required based on page content
./hweb --session conditional --url "https://app.com"
./hweb --session conditional --assert-js "(function() {
  if (document.querySelector('.login-form')) {
    return document.querySelector('#username') !== null;
  }
  return document.querySelector('.dashboard') !== null;
})()"
```

### Data Validation
```bash
# Validate API response data in web interface
./hweb --session api --url "https://api-dashboard.com"
./hweb --session api --assert-js "
  const data = JSON.parse(document.querySelector('#api-response').innerText);
  return data.users && data.users.length > 0 && data.status === 'success'
"
```

### Performance Assertions
```bash
# Check page load performance
./hweb --session perf --url "https://app.com"
./hweb --session perf --assert-js "window.performance.timing.loadEventEnd - window.performance.timing.navigationStart < 3000"
./hweb --session perf --assert-js "document.querySelectorAll('img[data-src]').length === 0" --message "All images should be loaded"
```

## Exit Codes and Error Handling

### Exit Code Reference
- `0`: All assertions passed
- `1`: One or more assertions failed
- `2`: Error during assertion execution

### Error Handling Examples
```bash
# Capture and handle specific assertion failures
./hweb --session test --assert-exists ".critical-element"
if [ $? -eq 1 ]; then
    echo "Critical element missing - taking screenshot for debugging"
    ./hweb --session test --screenshot "error-debug.png"
fi

# Multiple assertion strategy with early exit
./hweb --session multi \
  --url "https://app.com" \
  --assert-exists ".app-container" \
  --assert-js "window.APP_VERSION !== undefined" \
  --assert-count ".loading" "==0" \
  --assert-text ".status" "Ready"

# Exit code will be 1 if any assertion fails

## Test Failures Analysis

For a detailed breakdown of the current test failures, see [CurrentScriptTestFailures.txt](CurrentScriptTestFailures.txt).

---

# HeadlessWeb Test Failures Analysis

## Overview
Comprehensive test results show 83% success rate (5/6 modules passed). This document details all failing tests that need to be fixed.

## Test Module Status
- ✅ **test_navigation.sh** - PASSED
- ❌ **test_screenshot.sh** - PASSED (1 test failed but module passed)
- ❌ **test_assertions.sh** - FAILED (3/27 tests failed)
- ✅ **test_forms.sh** - PASSED (4 tests failed but module passed) 
- ✅ **test_javascript.sh** - PASSED (6 tests failed but module passed)
- ✅ **test_sessions.sh** - PASSED (3 tests failed but module passed)

## Critical Failures by Category

### 1. Screenshot Module Failures
**Module Status**: PASSED (1 failure)

#### Test: Screenshot Error Handling
- **Issue**: Invalid path error not handled properly
- **Expected**: Proper error handling for invalid screenshot paths
- **Status**: ✗ FAIL
- **Priority**: HIGH

### 2. Assertion Module Failures  
**Module Status**: FAILED (3/27 tests failed)

#### Test 12: Count - Zero Elements
- **Command**: `--assert-count '.nonexistent' '==0'`
- **Expected Exit Code**: 0
- **Actual Exit Code**: 1
- **Issue**: Zero element count assertion failing when it should pass
- **Priority**: HIGH

#### Test 22: Custom Message
- **Command**: `--assert-exists 'h1' --message 'Page title should exist'`
- **Expected**: Output contains 'Page title should exist'
- **Actual**: 'PASS: exists (h1) [2ms]'
- **Issue**: Custom assertion messages not being displayed
- **Priority**: MEDIUM

#### Test 24: Error - Invalid Selector
- **Command**: `--assert-exists 'invalid[selector'`
- **Expected Exit Code**: 2
- **Actual Exit Code**: 1
- **Issue**: Invalid CSS selector should return exit code 2, not 1
- **Priority**: MEDIUM

### 3. Form Module Failures
**Module Status**: PASSED (4 failures)

#### Form Submission Status
- **Issue**: Form submission status not properly detected
- **Expected**: 'Form submitted'
- **Actual**: '' (empty)
- **Priority**: MEDIUM

#### Form Validation - Empty Form
- **Issue**: Empty form validation status not detected
- **Expected**: 'Invalid' 
- **Actual**: '' (empty)
- **Priority**: MEDIUM

#### Form Validation - Valid Form
- **Issue**: Valid form validation status not detected
- **Expected**: 'Valid'
- **Actual**: '' (empty)
- **Priority**: MEDIUM

#### Form Error Handling
- **Issue**: Invalid form field selector not handled properly
- **Status**: ✗ FAIL
- **Priority**: LOW

### 4. JavaScript Module Failures
**Module Status**: PASSED (6 failures)

#### DOM Modification Persistence
- **Test**: DOM text modification result
- **Expected**: 'Modified text'
- **Actual**: 'Original text content'
- **Issue**: DOM modifications not persisting between command executions
- **Priority**: MEDIUM

#### Dynamic Element Creation
- **Expected**: 'Dynamically created'
- **Actual**: '' (empty)
- **Issue**: Dynamically created DOM elements not accessible
- **Priority**: MEDIUM

#### Event Handler Result
- **Expected**: 'Button clicked via JS'
- **Actual**: '' (empty)
- **Issue**: JavaScript event handlers not executing properly
- **Priority**: MEDIUM

#### Return Type Handling Issues
- **Undefined Return**: Expected 'undefined', Actual '' (empty)
- **Null Return**: Expected 'null', Actual '' (empty)  
- **Number Precision**: Expected '42.5', Actual '42.500000'
- **Priority**: LOW

### 5. Session Module Failures
**Module Status**: PASSED (3 failures)

#### localStorage Isolation Issues
- **localStorage isolation A**: Expected 'data_a', Actual 'data_c'
- **localStorage isolation B**: Expected 'data_b', Actual 'data_c'
- **Issue**: localStorage data bleeding between sessions
- **Priority**: MEDIUM

#### Session Listing Issues
- **Session A not found in list**: Sessions not properly listed
- **Session B not found in list**: Sessions not properly listed
- **Priority**: LOW

## Fix Priority Summary

### HIGH Priority (Must Fix)
1. Screenshot invalid path error handling
2. Assertion zero element count logic
3. Assertion invalid selector exit codes

### MEDIUM Priority (Should Fix)
4. Form validation status detection
5. JavaScript DOM modification persistence
6. Session localStorage isolation
7. Assertion custom message display

### LOW Priority (Nice to Fix)
8. JavaScript return type formatting
9. Session listing completeness
10. Form error handling edge cases

## Next Steps
1. Start with HIGH priority fixes
2. Test each fix individually 
3. Run comprehensive test suite after each major fix
4. Document any architectural changes needed
