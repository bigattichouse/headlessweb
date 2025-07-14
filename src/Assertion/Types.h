#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace Assertion {

enum class Result {
    PASS = 0,
    FAIL = 1,
    ERROR = 2
};

enum class ComparisonOperator {
    EQUALS,
    NOT_EQUALS,
    GREATER_THAN,
    LESS_THAN,
    GREATER_EQUAL,
    LESS_EQUAL,
    CONTAINS,
    NOT_CONTAINS,
    REGEX_MATCH
};

struct Command {
    std::string type;              // "exists", "text", "count", "js"
    std::string selector;          // CSS selector or JS expression
    std::string expected_value;    // Expected value for comparison
    std::string custom_message;    // Optional user message
    ComparisonOperator op;         // Comparison operator
    bool json_output;              // Format as JSON
    bool silent;                   // Don't print PASS/FAIL, just exit code
    bool case_sensitive;           // For text comparisons
    int timeout_ms;                // Timeout for element waiting
};

struct TestResult {
    std::string assertion_type;
    std::string selector;
    std::string expected;
    std::string actual;
    Result result;
    std::string message;
    std::chrono::milliseconds duration;
    std::string error_details;
};

struct SuiteResult {
    std::string suite_name;
    std::vector<TestResult> test_results;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    int total_tests;
    int passed_tests;
    int failed_tests;
    int error_tests;
};

} // namespace Assertion
