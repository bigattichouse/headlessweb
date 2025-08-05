# Event-Driven Testing Guide

## Phase 6: Replacing Blocking Patterns in Tests

This guide shows how to replace blocking `std::this_thread::sleep_for()` calls in tests with condition-based waiting patterns for better reliability and performance.

## Quick Reference

### Common Replacements

| Old Pattern | New Pattern |
|-------------|-------------|
| `std::this_thread::sleep_for(std::chrono::milliseconds(1000));` | `YIELD_TO_BROWSER();` or `WAIT_FOR(condition, timeout)` |
| Fixed retry loops with sleeps | `TestUtils::TestWaitUtilities::waitForCondition()` |
| Polling for element existence | `TestUtils::TestWaitUtilities::waitForElementExists()` |
| Fixed delays after form operations | `TestUtils::TestWaitUtilities::waitForInputFilled()` |
| Fixed delays after downloads | `TestUtils::TestWaitUtilities::waitForDownloadComplete()` |

### Essential Headers

```cpp
#include "../utils/TestWaitUtilities.h"

// Use these macros for common patterns:
// WAIT_FOR(condition, timeout_ms)
// WAIT_FOR_ELEMENT(selector, timeout_ms) 
// WAIT_FOR_ASSERTION(assertion, timeout_ms, message)
// TEST_SCOPE(name)
// YIELD_TO_BROWSER()
```

## Detailed Replacement Patterns

### 1. Page Loading and Setup

**❌ Old Pattern:**
```cpp
void SetUp() override {
    browser_->loadUri("about:blank");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // BLOCKING
    
    // More setup with fixed delays...
}
```

**✅ New Pattern:**
```cpp
void SetUp() override {
    TEST_SCOPE("TestClass SetUp");
    
    browser_->loadUri("about:blank");
    ASSERT_TRUE(WAIT_FOR(
        browser_->waitForNavigation(100),
        5000
    )) << "Failed to load blank page";
    
    ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForBrowserReady(10000))
        << "Browser not ready after setup";
}
```

### 2. JavaScript Execution Readiness

**❌ Old Pattern:**
```cpp
// Retry JavaScript execution with fixed delays
for (int i = 0; i < 5; i++) {
    std::string result = browser_->executeJavascriptSync("return 'test';");
    if (result == "test") break;
    if (i == 4) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // BLOCKING
}
```

**✅ New Pattern:**
```cpp
// Use condition-based JavaScript readiness
ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForJavaScriptReady(5000))
    << "JavaScript not ready";

// Or use specific condition waiting
ASSERT_TRUE(WAIT_FOR(
    browser_->executeJavascriptSync("return 'test';") == "test",
    3000
)) << "JavaScript execution not working";
```

### 3. DOM Element Readiness

**❌ Old Pattern:**
```cpp
// Check for required elements with fixed delays
if (!required_elements.empty()) {
    for (int i = 0; i < 5; i++) {
        bool all_elements_ready = true;
        for (const auto& element : required_elements) {
            std::string element_check = browser_->executeJavascriptSync(
                "return document.querySelector('" + element + "') !== null;"
            );
            if (element_check != "true") {
                all_elements_ready = false;
                break;
            }
        }
        if (all_elements_ready) break;
        if (i == 4) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // BLOCKING
    }
}
```

**✅ New Pattern:**
```cpp
// Use element-specific waiting
for (const auto& element : required_elements) {
    ASSERT_TRUE(WAIT_FOR_ELEMENT(element, 5000))
        << "Required element not found: " << element;
}

// Or combine into single condition
ASSERT_TRUE(WAIT_FOR([&]() {
    for (const auto& element : required_elements) {
        std::string check = browser_->executeJavascriptSync(
            "return document.querySelector('" + element + "') !== null;"
        );
        if (check != "true") return false;
    }
    return true;
}, 5000)) << "Not all required elements found";
```

### 4. Form Interactions

**❌ Old Pattern:**
```cpp
browser_->fillInput("#name-input", "Test User");
std::this_thread::sleep_for(std::chrono::milliseconds(500)); // BLOCKING

browser_->checkElement("#agree-checkbox");
std::this_thread::sleep_for(std::chrono::milliseconds(300)); // BLOCKING

browser_->clickElement("#submit-btn");
std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // BLOCKING
```

**✅ New Pattern:**
```cpp
browser_->fillInput("#name-input", "Test User");
ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForInputFilled("#name-input", 3000))
    << "Input fill not completed";

browser_->checkElement("#agree-checkbox");
ASSERT_TRUE(WAIT_FOR(
    browser_->executeJavascriptSync("return document.querySelector('#agree-checkbox').checked;") == "true",
    3000
)) << "Checkbox not checked";

browser_->clickElement("#submit-btn");
ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForFormSubmitted("#test-form", 10000))
    << "Form submission not completed";
```

### 5. Download Operations

**❌ Old Pattern:**
```cpp
browser_->clickElement("#download-link");
std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // BLOCKING

// Polling for download completion
bool download_found = false;
for (int i = 0; i < 30; i++) {
    if (/* check file exists */) {
        download_found = true;
        break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // BLOCKING
}
```

**✅ New Pattern:**
```cpp
browser_->clickElement("#download-link");
YIELD_TO_BROWSER(); // Brief synchronization

ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForDownloadComplete("test.txt", 30000))
    << "Download not completed within timeout";
```

### 6. Network Operations

**❌ Old Pattern:**
```cpp
browser_->clickElement("#fetch-btn");
std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // BLOCKING

// Check result
std::string result = browser_->executeJavascriptSync("return document.getElementById('result').textContent;");
```

**✅ New Pattern:**
```cpp
browser_->clickElement("#fetch-btn");

// Wait for network idle
ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForNetworkIdle(500, 10000))
    << "Network requests not completed";

// Wait for specific result condition
ASSERT_TRUE(WAIT_FOR(
    browser_->executeJavascriptSync("return document.getElementById('result').textContent;") == "expected_response",
    5000
)) << "Network response not received";
```

### 7. Test Cleanup and Teardown

**❌ Old Pattern:**
```cpp
void TearDown() override {
    // Reset browser state
    browser_->loadUri("about:blank");
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // BLOCKING
    
    // Cleanup resources
    temp_dir.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // BLOCKING
}
```

**✅ New Pattern:**
```cpp
void TearDown() override {
    // Reset browser state with condition waiting
    browser_->loadUri("about:blank");
    WAIT_FOR(browser_->waitForNavigation(100), 3000);
    
    // Cleanup resources
    temp_dir.reset();
    YIELD_TO_BROWSER(); // Process any pending events
}
```

## Advanced Patterns

### Custom Condition Waiting

```cpp
// Wait for complex conditions
ASSERT_TRUE(WAIT_FOR([&]() {
    // Custom condition logic
    std::string state = browser_->executeJavascriptSync("return myApp.getState();");
    return state == "ready" && some_other_condition();
}, 10000)) << "Custom condition not met";
```

### Performance Measurement

```cpp
// Measure operation performance
auto operation_time = TestUtils::TestWaitUtilities::measureOperationTime([&]() {
    // Test operation
    browser_->performSomeOperation();
    WAIT_FOR(operationComplete(), 5000);
});

EXPECT_LT(operation_time.count(), 2000) << "Operation too slow: " << operation_time.count() << "ms";
```

### Test Scoping and Logging

```cpp
TEST_F(MyTestClass, MyTestCase) {
    TEST_SCOPE("MyTestCase");  // Automatic timing and cleanup
    
    TestUtils::TestWaitUtilities::logTestStep("Starting complex operation");
    // ... test code ...
    
    TestUtils::TestWaitUtilities::logTestStep("Operation completed");
}
```

## Migration Strategy

### Phase 1: Identify Blocking Patterns
1. Search for `std::this_thread::sleep_for` in test files
2. Identify retry loops with fixed delays
3. Find fixed delays after operations

### Phase 2: Replace Common Patterns
1. Start with setup/teardown methods
2. Replace form interaction delays
3. Replace navigation delays
4. Replace assertion retry loops

### Phase 3: Implement Custom Conditions
1. Create specific condition functions for complex scenarios
2. Use `TestUtils::TestWaitUtilities::waitForCondition()` for custom logic
3. Add proper error messages for debugging

### Phase 4: Validate Performance
1. Measure test execution times before/after
2. Ensure tests are more reliable (fewer flaky failures)
3. Verify test isolation (no side effects)

## Benefits

1. **Reliability**: Tests wait for actual conditions instead of guessing timing
2. **Performance**: Tests complete as soon as conditions are met
3. **Maintainability**: Clear intent with descriptive error messages
4. **Scalability**: Works across different system speeds and loads
5. **Debugging**: Better failure messages and timing information

## Common Pitfalls

1. **Over-waiting**: Don't set unnecessarily long timeouts
2. **Under-waiting**: Ensure timeouts are reasonable for slow systems
3. **Missing conditions**: Make sure conditions actually check what you need
4. **Resource leaks**: Use RAII patterns (TEST_SCOPE) for cleanup
5. **Flaky conditions**: Ensure conditions are deterministic and repeatable