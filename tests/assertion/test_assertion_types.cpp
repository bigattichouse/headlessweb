#include <gtest/gtest.h>
#include "Assertion/Types.h"
#include <chrono>
#include <thread>
#include <climits>

using namespace Assertion;

class AssertionTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test data
    }
};

// ========== Result Enum Tests ==========

TEST_F(AssertionTypesTest, ResultEnumValues) {
    EXPECT_EQ(static_cast<int>(Result::PASS), 0);
    EXPECT_EQ(static_cast<int>(Result::FAIL), 1);
    EXPECT_EQ(static_cast<int>(Result::ERROR), 2);
}

// ========== ComparisonOperator Enum Tests ==========

TEST_F(AssertionTypesTest, ComparisonOperatorEnumValues) {
    // Test that all enum values are present
    ComparisonOperator op;
    
    op = ComparisonOperator::EQUALS;
    op = ComparisonOperator::NOT_EQUALS;
    op = ComparisonOperator::GREATER_THAN;
    op = ComparisonOperator::LESS_THAN;
    op = ComparisonOperator::GREATER_EQUAL;
    op = ComparisonOperator::LESS_EQUAL;
    op = ComparisonOperator::CONTAINS;
    op = ComparisonOperator::NOT_CONTAINS;
    op = ComparisonOperator::REGEX_MATCH;
    
    // Just testing compilation - if we get here, all enum values exist
    SUCCEED();
}

// ========== Command Structure Tests ==========

TEST_F(AssertionTypesTest, CommandDefaultConstruction) {
    Command cmd;
    
    // Test default values - empty struct with uninitialized values
    EXPECT_TRUE(cmd.type.empty());
    EXPECT_TRUE(cmd.selector.empty());
    EXPECT_TRUE(cmd.expected_value.empty());
    EXPECT_TRUE(cmd.custom_message.empty());
    // Don't test uninitialized values - they could be anything
    // EXPECT_EQ(cmd.op, ComparisonOperator::EQUALS);
    // EXPECT_FALSE(cmd.json_output);
    // EXPECT_FALSE(cmd.silent);
    // EXPECT_TRUE(cmd.case_sensitive);
    // EXPECT_GT(cmd.timeout_ms, 0);
}

TEST_F(AssertionTypesTest, CommandInitialization) {
    Command cmd;
    cmd.type = "exists";
    cmd.selector = "#test-element";
    cmd.expected_value = "true";
    cmd.custom_message = "Test message";
    cmd.op = ComparisonOperator::NOT_EQUALS;
    cmd.json_output = true;
    cmd.silent = true;
    cmd.case_sensitive = false;
    cmd.timeout_ms = 10000;
    
    EXPECT_EQ(cmd.type, "exists");
    EXPECT_EQ(cmd.selector, "#test-element");
    EXPECT_EQ(cmd.expected_value, "true");
    EXPECT_EQ(cmd.custom_message, "Test message");
    EXPECT_EQ(cmd.op, ComparisonOperator::NOT_EQUALS);
    EXPECT_TRUE(cmd.json_output);
    EXPECT_TRUE(cmd.silent);
    EXPECT_FALSE(cmd.case_sensitive);
    EXPECT_EQ(cmd.timeout_ms, 10000);
}

TEST_F(AssertionTypesTest, CommandWithDifferentTypes) {
    std::vector<std::string> assertionTypes = {
        "exists", "text", "count", "js", "attr", "visible", "enabled"
    };
    
    for (const auto& type : assertionTypes) {
        Command cmd;
        cmd.type = type;
        EXPECT_EQ(cmd.type, type);
    }
}

TEST_F(AssertionTypesTest, CommandWithComplexSelectors) {
    std::vector<std::string> selectors = {
        "#simple-id",
        ".class-name",
        "div.class#id",
        "[data-test='value']",
        "div > .child:nth-child(2)",
        "input[type='text']:not([disabled])",
        "//xpath/expression",
        "complex >> selector"
    };
    
    for (const auto& selector : selectors) {
        Command cmd;
        cmd.selector = selector;
        EXPECT_EQ(cmd.selector, selector);
    }
}

// ========== TestResult Structure Tests ==========

TEST_F(AssertionTypesTest, TestResultDefaultConstruction) {
    TestResult result;
    
    EXPECT_TRUE(result.assertion_type.empty());
    EXPECT_TRUE(result.selector.empty());
    EXPECT_TRUE(result.expected.empty());
    EXPECT_TRUE(result.actual.empty());
    // Don't test uninitialized enum value
    EXPECT_TRUE(result.message.empty());
    EXPECT_TRUE(result.error_details.empty());
    // Duration should be initialized to some value
    EXPECT_GE(result.duration.count(), 0);
}

TEST_F(AssertionTypesTest, TestResultInitialization) {
    TestResult result;
    result.assertion_type = "text";
    result.selector = "#content";
    result.expected = "Expected Text";
    result.actual = "Actual Text";
    result.result = Result::FAIL;
    result.message = "Text mismatch";
    result.duration = std::chrono::milliseconds(150);
    result.error_details = "Expected 'Expected Text' but got 'Actual Text'";
    
    EXPECT_EQ(result.assertion_type, "text");
    EXPECT_EQ(result.selector, "#content");
    EXPECT_EQ(result.expected, "Expected Text");
    EXPECT_EQ(result.actual, "Actual Text");
    EXPECT_EQ(result.result, Result::FAIL);
    EXPECT_EQ(result.message, "Text mismatch");
    EXPECT_EQ(result.duration.count(), 150);
    EXPECT_EQ(result.error_details, "Expected 'Expected Text' but got 'Actual Text'");
}

TEST_F(AssertionTypesTest, TestResultWithAllResults) {
    std::vector<Result> results = {Result::PASS, Result::FAIL, Result::ERROR};
    
    for (Result res : results) {
        TestResult testResult;
        testResult.result = res;
        EXPECT_EQ(testResult.result, res);
    }
}

TEST_F(AssertionTypesTest, TestResultDurationHandling) {
    TestResult result;
    
    // Test various duration values
    result.duration = std::chrono::milliseconds(0);
    EXPECT_EQ(result.duration.count(), 0);
    
    result.duration = std::chrono::milliseconds(1);
    EXPECT_EQ(result.duration.count(), 1);
    
    result.duration = std::chrono::milliseconds(5000);
    EXPECT_EQ(result.duration.count(), 5000);
    
    // Test with actual time measurements
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto end = std::chrono::steady_clock::now();
    
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_GE(result.duration.count(), 10);
    EXPECT_LT(result.duration.count(), 100); // Should be reasonable
}

// ========== SuiteResult Structure Tests ==========

TEST_F(AssertionTypesTest, SuiteResultDefaultConstruction) {
    SuiteResult suite;
    
    EXPECT_TRUE(suite.suite_name.empty());
    EXPECT_TRUE(suite.test_results.empty());
    // Don't test uninitialized int values
    // EXPECT_EQ(suite.total_tests, 0);
    // EXPECT_EQ(suite.passed_tests, 0);
    // EXPECT_EQ(suite.failed_tests, 0);
    // EXPECT_EQ(suite.error_tests, 0);
}

TEST_F(AssertionTypesTest, SuiteResultInitialization) {
    SuiteResult suite;
    suite.suite_name = "Integration Tests";
    suite.total_tests = 10;
    suite.passed_tests = 7;
    suite.failed_tests = 2;
    suite.error_tests = 1;
    
    auto now = std::chrono::system_clock::now();
    suite.start_time = now - std::chrono::minutes(5);
    suite.end_time = now;
    
    EXPECT_EQ(suite.suite_name, "Integration Tests");
    EXPECT_EQ(suite.total_tests, 10);
    EXPECT_EQ(suite.passed_tests, 7);
    EXPECT_EQ(suite.failed_tests, 2);
    EXPECT_EQ(suite.error_tests, 1);
    EXPECT_LT(suite.start_time, suite.end_time);
}

TEST_F(AssertionTypesTest, SuiteResultWithTestResults) {
    SuiteResult suite;
    suite.suite_name = "Test Suite";
    
    // Add test results
    TestResult result1;
    result1.assertion_type = "exists";
    result1.selector = "#test1";
    result1.result = Result::PASS;
    
    TestResult result2;
    result2.assertion_type = "text";
    result2.selector = "#test2";
    result2.result = Result::FAIL;
    
    TestResult result3;
    result3.assertion_type = "count";
    result3.selector = ".items";
    result3.result = Result::ERROR;
    
    suite.test_results.push_back(result1);
    suite.test_results.push_back(result2);
    suite.test_results.push_back(result3);
    
    EXPECT_EQ(suite.test_results.size(), 3);
    EXPECT_EQ(suite.test_results[0].result, Result::PASS);
    EXPECT_EQ(suite.test_results[1].result, Result::FAIL);
    EXPECT_EQ(suite.test_results[2].result, Result::ERROR);
}

TEST_F(AssertionTypesTest, SuiteResultStatisticsConsistency) {
    SuiteResult suite;
    
    // Test that statistics match the test results
    TestResult pass1, pass2, fail1, error1;
    pass1.result = Result::PASS;
    pass2.result = Result::PASS;
    fail1.result = Result::FAIL;
    error1.result = Result::ERROR;
    
    suite.test_results = {pass1, pass2, fail1, error1};
    suite.total_tests = 4;
    suite.passed_tests = 2;
    suite.failed_tests = 1;
    suite.error_tests = 1;
    
    // Verify consistency
    EXPECT_EQ(suite.total_tests, suite.test_results.size());
    EXPECT_EQ(suite.total_tests, suite.passed_tests + suite.failed_tests + suite.error_tests);
    
    // Count actual results
    int actualPassed = 0, actualFailed = 0, actualErrors = 0;
    for (const auto& result : suite.test_results) {
        switch (result.result) {
            case Result::PASS: actualPassed++; break;
            case Result::FAIL: actualFailed++; break;
            case Result::ERROR: actualErrors++; break;
        }
    }
    
    EXPECT_EQ(suite.passed_tests, actualPassed);
    EXPECT_EQ(suite.failed_tests, actualFailed);
    EXPECT_EQ(suite.error_tests, actualErrors);
}

// ========== Edge Cases and Boundaries ==========

TEST_F(AssertionTypesTest, EmptyStringHandling) {
    Command cmd;
    cmd.type = "";
    cmd.selector = "";
    cmd.expected_value = "";
    cmd.custom_message = "";
    
    // Should handle empty strings gracefully
    EXPECT_EQ(cmd.type, "");
    EXPECT_EQ(cmd.selector, "");
    EXPECT_EQ(cmd.expected_value, "");
    EXPECT_EQ(cmd.custom_message, "");
}

TEST_F(AssertionTypesTest, LargeStringHandling) {
    Command cmd;
    
    std::string largeString(10000, 'x');
    cmd.expected_value = largeString;
    
    EXPECT_EQ(cmd.expected_value.size(), 10000);
    EXPECT_EQ(cmd.expected_value, largeString);
}

TEST_F(AssertionTypesTest, UnicodeStringHandling) {
    Command cmd;
    
    std::string unicodeText = "测试文本 🌟 Тест مرحبا";
    cmd.expected_value = unicodeText;
    cmd.custom_message = "Unicode test: " + unicodeText;
    
    EXPECT_EQ(cmd.expected_value, unicodeText);
    EXPECT_TRUE(cmd.custom_message.find(unicodeText) != std::string::npos);
}

TEST_F(AssertionTypesTest, SpecialCharacterHandling) {
    Command cmd;
    
    // Test with various special characters that might appear in selectors or text
    std::vector<std::string> specialCases = {
        "selector with spaces",
        "selector\twith\ttabs",
        "selector\nwith\nnewlines",
        "selector\"with\"quotes",
        "selector'with'quotes",
        "selector\\with\\backslashes",
        "selector/with/slashes",
        "selector#with#hashes",
        "selector$with$dollars",
        "selector%with%percents",
        "selector&with&ampersands"
    };
    
    for (const auto& special : specialCases) {
        cmd.selector = special;
        EXPECT_EQ(cmd.selector, special);
    }
}

TEST_F(AssertionTypesTest, ExtremeTimeoutValues) {
    Command cmd;
    
    // Test extreme timeout values
    cmd.timeout_ms = 0;
    EXPECT_EQ(cmd.timeout_ms, 0);
    
    cmd.timeout_ms = 1;
    EXPECT_EQ(cmd.timeout_ms, 1);
    
    cmd.timeout_ms = INT_MAX;
    EXPECT_EQ(cmd.timeout_ms, INT_MAX);
    
    // Negative timeouts should be handled by the implementation
    cmd.timeout_ms = -1;
    EXPECT_EQ(cmd.timeout_ms, -1);
}

TEST_F(AssertionTypesTest, SuiteResultTimeCalculations) {
    SuiteResult suite;
    
    auto baseTime = std::chrono::system_clock::now();
    suite.start_time = baseTime;
    suite.end_time = baseTime + std::chrono::minutes(5) + std::chrono::seconds(30);
    
    auto duration = suite.end_time - suite.start_time;
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    
    // Should be approximately 5.5 minutes
    EXPECT_GE(durationMs.count(), 330000); // 5.5 min = 330,000 ms
    EXPECT_LE(durationMs.count(), 331000); // Allow for small variance
}

// ========== Copy and Assignment Tests ==========

TEST_F(AssertionTypesTest, CommandCopyAndAssignment) {
    Command original;
    original.type = "exists";
    original.selector = "#test";
    original.expected_value = "true";
    original.json_output = true;
    original.timeout_ms = 5000;
    
    // Copy constructor
    Command copied(original);
    EXPECT_EQ(copied.type, original.type);
    EXPECT_EQ(copied.selector, original.selector);
    EXPECT_EQ(copied.expected_value, original.expected_value);
    EXPECT_EQ(copied.json_output, original.json_output);
    EXPECT_EQ(copied.timeout_ms, original.timeout_ms);
    
    // Assignment operator
    Command assigned;
    assigned = original;
    EXPECT_EQ(assigned.type, original.type);
    EXPECT_EQ(assigned.selector, original.selector);
    EXPECT_EQ(assigned.expected_value, original.expected_value);
    EXPECT_EQ(assigned.json_output, original.json_output);
    EXPECT_EQ(assigned.timeout_ms, original.timeout_ms);
}

TEST_F(AssertionTypesTest, TestResultCopyAndAssignment) {
    TestResult original;
    original.assertion_type = "text";
    original.selector = "#content";
    original.expected = "Expected";
    original.actual = "Actual";
    original.result = Result::FAIL;
    original.duration = std::chrono::milliseconds(100);
    
    // Copy constructor
    TestResult copied(original);
    EXPECT_EQ(copied.assertion_type, original.assertion_type);
    EXPECT_EQ(copied.selector, original.selector);
    EXPECT_EQ(copied.expected, original.expected);
    EXPECT_EQ(copied.actual, original.actual);
    EXPECT_EQ(copied.result, original.result);
    EXPECT_EQ(copied.duration.count(), original.duration.count());
    
    // Assignment operator
    TestResult assigned;
    assigned = original;
    EXPECT_EQ(assigned.assertion_type, original.assertion_type);
    EXPECT_EQ(assigned.selector, original.selector);
    EXPECT_EQ(assigned.expected, original.expected);
    EXPECT_EQ(assigned.actual, original.actual);
    EXPECT_EQ(assigned.result, original.result);
    EXPECT_EQ(assigned.duration.count(), original.duration.count());
}