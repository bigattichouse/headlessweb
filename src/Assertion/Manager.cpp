#include "Manager.h"
#include <iostream>
#include <regex>
#include <algorithm>
#include <cctype>

namespace Assertion {

Manager::Manager() : silent_mode(false), json_output(false) {
}

Manager::~Manager() = default;

Result Manager::assertExists(Browser& browser, const Command& cmd) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Parse expected value (true/false)
        bool expected = (cmd.expected_value == "true" || cmd.expected_value == "1" || 
                        cmd.expected_value == "yes" || cmd.expected_value.empty());
        
        // Check if element exists (with timeout if specified)
        int existence_result = 0; // 0 = doesn't exist, 1 = exists, -1 = invalid selector
        
        if (cmd.timeout_ms > 0) {
            // Use browser's event-driven waiting first
            bool found_via_wait = browser.waitForSelector(cmd.selector, cmd.timeout_ms);
            if (found_via_wait) {
                existence_result = 1;
            } else {
                // Double-check with validation for negative assertions
                existence_result = browser.elementExistsWithValidation(cmd.selector);
            }
        } else {
            existence_result = browser.elementExistsWithValidation(cmd.selector);
        }
        
        // Handle invalid selector case (should return error exit code 2)
        if (existence_result == -1) {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            TestResult test_result = createResult(cmd, Result::ERROR, "false", "Invalid CSS selector");
            test_result.duration = duration;
            
            if (!silent_mode) {
                outputResult(test_result);
            }
            
            addResult(test_result);
            return Result::ERROR; // This should map to exit code 2
        }
        
        bool exists = (existence_result == 1);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        Result result = (exists == expected) ? Result::PASS : Result::FAIL;
        
        TestResult test_result = createResult(cmd, result, exists ? "true" : "false");
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return result;
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        TestResult test_result = createResult(cmd, Result::ERROR, "", e.what());
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return Result::ERROR;
    }
}

Result Manager::assertText(Browser& browser, const Command& cmd) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Get actual text content
        std::string actual_text = browser.getInnerText(cmd.selector);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Parse operator from expected value
        ComparisonOperator op = cmd.op;
        std::string expected = cmd.expected_value;
        
        // If no explicit operator specified in command, try to parse from value
        if (op == ComparisonOperator::EQUALS && cmd.expected_value.find_first_of("><!=~") != std::string::npos) {
            expected = extractOperatorFromValue(cmd.expected_value, op);
        }
        
        bool matches = compareValues(actual_text, expected, op, cmd.case_sensitive);
        
        Result result = matches ? Result::PASS : Result::FAIL;
        
        TestResult test_result = createResult(cmd, result, actual_text);
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return result;
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        TestResult test_result = createResult(cmd, Result::ERROR, "", e.what());
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return Result::ERROR;
    }
}

Result Manager::assertCount(Browser& browser, const Command& cmd) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Get actual count
        int actual_count = browser.countElements(cmd.selector);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Parse operator and expected count
        ComparisonOperator op = cmd.op;
        std::string expected_str = cmd.expected_value;
        
        if (op == ComparisonOperator::EQUALS) {
            expected_str = extractOperatorFromValue(cmd.expected_value, op);
        }
        
        // Convert to string for comparison
        std::string actual_str = std::to_string(actual_count);
        
        bool matches = compareValues(actual_str, expected_str, op, true);
        
        Result result = matches ? Result::PASS : Result::FAIL;
        
        TestResult test_result = createResult(cmd, result, actual_str);
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return result;
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        TestResult test_result = createResult(cmd, Result::ERROR, "", e.what());
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return Result::ERROR;
    }
}

Result Manager::assertJavaScript(Browser& browser, const Command& cmd) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Execute JavaScript and get result
        std::string js_result = browser.executeJavascriptSync(cmd.selector); // selector contains JS code
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Evaluate result as boolean
        bool actual_bool = false;
        if (js_result == "true" || js_result == "1") {
            actual_bool = true;
        } else if (js_result == "false" || js_result == "0") {
            actual_bool = false;
        } else {
            // Try to parse as number (non-zero is true)
            try {
                double num = std::stod(js_result);
                actual_bool = (num != 0.0);
            } catch (...) {
                // Non-empty string is true
                actual_bool = !js_result.empty();
            }
        }
        
        // Expected result
        bool expected_bool = (cmd.expected_value == "true" || cmd.expected_value == "1" || 
                             cmd.expected_value.empty());
        
        Result result = (actual_bool == expected_bool) ? Result::PASS : Result::FAIL;
        
        TestResult test_result = createResult(cmd, result, js_result);
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return result;
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        TestResult test_result = createResult(cmd, Result::ERROR, "", e.what());
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return Result::ERROR;
    }
}

Result Manager::assertElementValue(Browser& browser, const Command& cmd) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Get actual element value (for form inputs and other elements with value attribute)
        std::string actual_value = browser.getAttribute(cmd.selector, "value");
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Parse operator from expected value
        ComparisonOperator op = cmd.op;
        std::string expected = cmd.expected_value;
        
        // If no explicit operator specified in command, try to parse from value
        if (op == ComparisonOperator::EQUALS && cmd.expected_value.find_first_of("><!=~") != std::string::npos) {
            expected = extractOperatorFromValue(cmd.expected_value, op);
        }
        
        bool matches = compareValues(actual_value, expected, op, cmd.case_sensitive);
        
        Result result = matches ? Result::PASS : Result::FAIL;
        
        TestResult test_result = createResult(cmd, result, actual_value);
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return result;
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        TestResult test_result = createResult(cmd, Result::ERROR, "", e.what());
        test_result.duration = duration;
        
        if (!silent_mode) {
            outputResult(test_result);
        }
        
        addResult(test_result);
        return Result::ERROR;
    }
}

Result Manager::executeAssertion(Browser& browser, const Command& cmd) {
    if (cmd.type == "exists" || cmd.type == "element-exists") {
        return assertExists(browser, cmd);
    } else if (cmd.type == "text") {
        return assertText(browser, cmd);
    } else if (cmd.type == "value" || cmd.type == "element-value") {
        return assertElementValue(browser, cmd);
    } else if (cmd.type == "count") {
        return assertCount(browser, cmd);
    } else if (cmd.type == "javascript" || cmd.type == "js") {
        return assertJavaScript(browser, cmd);
    } else {
        TestResult test_result = createResult(cmd, Result::ERROR, "", "Unknown assertion type: " + cmd.type);
        if (!silent_mode) {
            outputResult(test_result);
        }
        addResult(test_result);
        return Result::ERROR;
    }
}

void Manager::startSuite(const std::string& name) {
    current_suite = std::make_unique<SuiteResult>();
    current_suite->suite_name = name;
    current_suite->start_time = std::chrono::system_clock::now();
    current_suite->total_tests = 0;
    current_suite->passed_tests = 0;
    current_suite->failed_tests = 0;
    current_suite->error_tests = 0;
    
    clearResults();
    
    if (!silent_mode && !json_output) {
        std::cout << "Starting test suite: " << name << std::endl;
    }
}

void Manager::endSuite(bool json_output, const std::string& format, bool suppress_exit) {
    if (!current_suite) {
        std::cerr << "Error: No active test suite to end" << std::endl;
        return;
    }
    
    current_suite->end_time = std::chrono::system_clock::now();
    current_suite->test_results = results;
    current_suite->total_tests = getTotalTests();
    current_suite->passed_tests = getPassedTests();
    current_suite->failed_tests = getFailedTests();
    current_suite->error_tests = getErrorTests();
    
    // Output results based on format
    if (format == "junit") {
        OutputFormatter::formatJUnitXML(*current_suite);
    } else {
        OutputFormatter::formatSuiteResult(*current_suite, json_output);
    }
    
    // Exit with appropriate code (unless suppressed for interface testing)
    if (!suppress_exit) {
        int exit_code = (current_suite->failed_tests > 0 || current_suite->error_tests > 0) ? 1 : 0;
        current_suite.reset();
        exit(exit_code);
    } else {
        current_suite.reset();
    }
}

bool Manager::isSuiteActive() const {
    return current_suite != nullptr;
}

void Manager::addResult(const TestResult& result) {
    results.push_back(result);
}

const std::vector<TestResult>& Manager::getResults() const {
    return results;
}

void Manager::clearResults() {
    results.clear();
}

int Manager::getTotalTests() const {
    return static_cast<int>(results.size());
}

int Manager::getPassedTests() const {
    return std::count_if(results.begin(), results.end(),
        [](const TestResult& r) { return r.result == Result::PASS; });
}

int Manager::getFailedTests() const {
    return std::count_if(results.begin(), results.end(),
        [](const TestResult& r) { return r.result == Result::FAIL; });
}

int Manager::getErrorTests() const {
    return std::count_if(results.begin(), results.end(),
        [](const TestResult& r) { return r.result == Result::ERROR; });
}

void Manager::setSilentMode(bool silent) {
    silent_mode = silent;
}

void Manager::setJsonOutput(bool json) {
    json_output = json;
}

TestResult Manager::createResult(const Command& cmd, Result result, 
                                const std::string& actual, const std::string& error) {
    TestResult test_result;
    test_result.assertion_type = cmd.type;
    test_result.selector = cmd.selector;
    test_result.expected = cmd.expected_value;
    test_result.actual = actual;
    test_result.result = result;
    test_result.message = cmd.custom_message;
    test_result.error_details = error;
    test_result.duration = std::chrono::milliseconds(0); // Will be set by caller
    
    return test_result;
}

bool Manager::compareValues(const std::string& actual, const std::string& expected, 
                           ComparisonOperator op, bool case_sensitive) {
    std::string actual_cmp = actual;
    std::string expected_cmp = expected;
    
    // Handle case sensitivity
    if (!case_sensitive) {
        std::transform(actual_cmp.begin(), actual_cmp.end(), actual_cmp.begin(), ::tolower);
        std::transform(expected_cmp.begin(), expected_cmp.end(), expected_cmp.begin(), ::tolower);
    }
    
    switch (op) {
        case ComparisonOperator::EQUALS:
            return actual_cmp == expected_cmp;
            
        case ComparisonOperator::NOT_EQUALS:
            return actual_cmp != expected_cmp;
            
        case ComparisonOperator::CONTAINS:
            return actual_cmp.find(expected_cmp) != std::string::npos;
            
        case ComparisonOperator::NOT_CONTAINS:
            return actual_cmp.find(expected_cmp) == std::string::npos;
            
        case ComparisonOperator::GREATER_THAN: {
            try {
                double actual_num = std::stod(actual);
                double expected_num = std::stod(expected);
                return actual_num > expected_num;
            } catch (...) {
                return actual_cmp > expected_cmp; // String comparison fallback
            }
        }
        
        case ComparisonOperator::LESS_THAN: {
            try {
                double actual_num = std::stod(actual);
                double expected_num = std::stod(expected);
                return actual_num < expected_num;
            } catch (...) {
                return actual_cmp < expected_cmp; // String comparison fallback
            }
        }
        
        case ComparisonOperator::GREATER_EQUAL: {
            try {
                double actual_num = std::stod(actual);
                double expected_num = std::stod(expected);
                return actual_num >= expected_num;
            } catch (...) {
                return actual_cmp >= expected_cmp; // String comparison fallback
            }
        }
        
        case ComparisonOperator::LESS_EQUAL: {
            try {
                double actual_num = std::stod(actual);
                double expected_num = std::stod(expected);
                return actual_num <= expected_num;
            } catch (...) {
                return actual_cmp <= expected_cmp; // String comparison fallback
            }
        }
        
        case ComparisonOperator::REGEX_MATCH: {
            try {
                std::regex pattern(expected);
                return std::regex_search(actual, pattern);
            } catch (const std::exception&) {
                return false; // Invalid regex
            }
        }
        
        default:
            return false;
    }
}

std::string Manager::extractOperatorFromValue(const std::string& value, ComparisonOperator& op) {
    // Parse operators like ">5", "!=hello", "~=pattern"
    if (value.length() >= 2) {
        std::string prefix = value.substr(0, 2);
        if (prefix == ">=") {
            op = ComparisonOperator::GREATER_EQUAL;
            return value.substr(2);
        } else if (prefix == "<=") {
            op = ComparisonOperator::LESS_EQUAL;
            return value.substr(2);
        } else if (prefix == "!=") {
            op = ComparisonOperator::NOT_EQUALS;
            return value.substr(2);
        } else if (prefix == "~=") {
            op = ComparisonOperator::REGEX_MATCH;
            return value.substr(2);
        } else if (prefix == "==") {
            op = ComparisonOperator::EQUALS;
            return value.substr(2);
        }
    }
    
    if (value.length() >= 1) {
        char first = value[0];
        if (first == '>') {
            op = ComparisonOperator::GREATER_THAN;
            return value.substr(1);
        } else if (first == '<') {
            op = ComparisonOperator::LESS_THAN;
            return value.substr(1);
        }
    }
    
    // Check for contains syntax
    if (value.find("contains:") == 0) {
        op = ComparisonOperator::CONTAINS;
        return value.substr(9);
    }
    
    // Default to equals
    op = ComparisonOperator::EQUALS;
    return value;
}

void Manager::outputResult(const TestResult& result) {
    OutputFormatter::formatResult(result, json_output);
}

} // namespace Assertion
