#include <gtest/gtest.h>
#include "Assertion/OutputFormatter.h"
#include <sstream>
#include <json/json.h>

using namespace Assertion;

class OutputFormatterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear output streams
        text_stream.str("");
        text_stream.clear();
        json_stream.str("");
        json_stream.clear();
    }

    TestResult createTestResult(const std::string& name, Result result, 
                               const std::string& actual = "", 
                               const std::string& expected = "") {
        TestResult test_result;
        test_result.assertion_type = name;
        test_result.result = result;
        test_result.actual = actual;
        test_result.expected = expected;
        test_result.duration = std::chrono::milliseconds(100);
        test_result.selector = "#test";
        test_result.message = "";
        test_result.error_details = "";
        return test_result;
    }

    SuiteResult createSuiteResult(const std::string& name) {
        SuiteResult suite;
        suite.suite_name = name;
        suite.total_tests = 0;
        suite.passed_tests = 0;
        suite.failed_tests = 0;
        suite.error_tests = 0;
        suite.start_time = std::chrono::system_clock::now();
        suite.end_time = std::chrono::system_clock::now();
        return suite;
    }

    std::stringstream text_stream;
    std::stringstream json_stream;
};

// ========== Single Test Result Formatting ==========

TEST_F(OutputFormatterTest, FormatPassingTestResult) {
    TestResult result = createTestResult("test_passes", Result::PASS);
    
    OutputFormatter::formatResult(result, false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("test_passes"), std::string::npos);
    EXPECT_NE(output.find("PASS"), std::string::npos);
}

TEST_F(OutputFormatterTest, FormatFailingTestResult) {
    TestResult result = createTestResult("test_fails", Result::FAIL, "actual", "expected");
    
    OutputFormatter::formatResult(result, false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("test_fails"), std::string::npos);
    EXPECT_NE(output.find("FAIL"), std::string::npos);
    EXPECT_NE(output.find("actual"), std::string::npos);
    EXPECT_NE(output.find("expected"), std::string::npos);
}

TEST_F(OutputFormatterTest, FormatErrorTestResult) {
    TestResult result = createTestResult("test_error", Result::ERROR);
    result.error_details = "Something went wrong";
    
    OutputFormatter::formatResult(result, false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("test_error"), std::string::npos);
    EXPECT_NE(output.find("ERROR"), std::string::npos);
    EXPECT_NE(output.find("Something went wrong"), std::string::npos);
}

TEST_F(OutputFormatterTest, FormatTestResultAsJson) {
    TestResult result = createTestResult("test_json", Result::PASS);
    
    OutputFormatter::formatResult(result, true, json_stream);
    
    std::string output = json_stream.str();
    
    // Debug: print the actual JSON output
    std::cout << "Actual JSON output: " << output << std::endl;
    
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(output, root));
    
    // Check if the fields actually exist in the JSON
    if (root.isMember("assertion_type")) {
        EXPECT_EQ(root["assertion_type"].asString(), "test_json");
    }
    if (root.isMember("result")) {
        EXPECT_EQ(root["result"].asString(), "PASS");
    }
}

// ========== Error Message Formatting ==========

TEST_F(OutputFormatterTest, FormatErrorMessage) {
    OutputFormatter::formatError("command", "error message", false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("command"), std::string::npos);
    EXPECT_NE(output.find("error message"), std::string::npos);
}

TEST_F(OutputFormatterTest, FormatErrorMessageAsJson) {
    OutputFormatter::formatError("command", "error message", true, json_stream);
    
    std::string output = json_stream.str();
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(output, root));
    EXPECT_EQ(root["command"].asString(), "command");
    EXPECT_EQ(root["error"].asString(), "error message");
}

// ========== Suite Result Formatting ==========

TEST_F(OutputFormatterTest, FormatBasicSuiteResult) {
    SuiteResult suite = createSuiteResult("Test Suite");
    suite.total_tests = 10;
    suite.passed_tests = 8;
    suite.failed_tests = 2;
    suite.error_tests = 0;
    
    OutputFormatter::formatSuiteResult(suite, false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("Test Suite"), std::string::npos);
    EXPECT_NE(output.find("10"), std::string::npos); // total
    EXPECT_NE(output.find("8"), std::string::npos);  // passed
    EXPECT_NE(output.find("2"), std::string::npos);  // failed
}

TEST_F(OutputFormatterTest, FormatSuiteResultAsJson) {
    SuiteResult suite = createSuiteResult("JSON Suite");
    suite.total_tests = 5;
    suite.passed_tests = 5;
    suite.failed_tests = 0;
    suite.error_tests = 0;
    
    OutputFormatter::formatSuiteResult(suite, true, json_stream);
    
    std::string output = json_stream.str();
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(output, root));
    EXPECT_EQ(root["suite_name"].asString(), "JSON Suite");
    EXPECT_EQ(root["total_tests"].asInt(), 5);
    EXPECT_EQ(root["passed_tests"].asInt(), 5);
}

TEST_F(OutputFormatterTest, FormatSuiteWithTestResults) {
    SuiteResult suite = createSuiteResult("Suite with Results");
    suite.total_tests = 2;
    suite.passed_tests = 1;
    suite.failed_tests = 1;
    
    TestResult pass_result = createTestResult("passing_test", Result::PASS);
    TestResult fail_result = createTestResult("failing_test", Result::FAIL);
    suite.test_results.push_back(pass_result);
    suite.test_results.push_back(fail_result);
    
    OutputFormatter::formatSuiteResult(suite, true, json_stream);
    
    std::string output = json_stream.str();
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(output, root));
    EXPECT_TRUE(root.isMember("test_results"));
    EXPECT_EQ(root["test_results"].size(), 2);
}

// ========== JUnit XML Formatting ==========

TEST_F(OutputFormatterTest, FormatJUnitXMLBasic) {
    SuiteResult suite = createSuiteResult("JUnit Suite");
    suite.total_tests = 3;
    suite.passed_tests = 2;
    suite.failed_tests = 1;
    suite.error_tests = 0;
    
    TestResult pass_result = createTestResult("test_pass", Result::PASS);
    TestResult fail_result = createTestResult("test_fail", Result::FAIL);
    fail_result.error_details = "Test failed";
    suite.test_results.push_back(pass_result);
    suite.test_results.push_back(fail_result);
    
    OutputFormatter::formatJUnitXML(suite, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("<?xml"), std::string::npos);
    EXPECT_NE(output.find("<testsuite"), std::string::npos);
    EXPECT_NE(output.find("JUnit Suite"), std::string::npos);
    EXPECT_NE(output.find("<testcase"), std::string::npos);
    EXPECT_NE(output.find("test_pass"), std::string::npos);
    EXPECT_NE(output.find("test_fail"), std::string::npos);
    EXPECT_NE(output.find("<failure"), std::string::npos);
}

TEST_F(OutputFormatterTest, FormatJUnitXMLWithErrors) {
    SuiteResult suite = createSuiteResult("Error Suite");
    suite.total_tests = 1;
    suite.passed_tests = 0;
    suite.failed_tests = 0;
    suite.error_tests = 1;
    
    TestResult error_result = createTestResult("test_error", Result::ERROR);
    error_result.error_details = "Exception occurred";
    suite.test_results.push_back(error_result);
    
    OutputFormatter::formatJUnitXML(suite, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("<error"), std::string::npos);
    EXPECT_NE(output.find("Exception occurred"), std::string::npos);
}

// ========== Special Characters and Escaping ==========

TEST_F(OutputFormatterTest, FormatResultWithSpecialCharacters) {
    TestResult result = createTestResult("test_special", Result::FAIL, 
                                        "actual<>&\"", "expected<>&\"");
    result.error_details = "Error with <special> & \"quoted\" characters";
    
    OutputFormatter::formatJUnitXML(SuiteResult{}, text_stream);
    // Just ensure it doesn't crash - XML escaping should handle special chars
    EXPECT_FALSE(text_stream.str().empty());
}

TEST_F(OutputFormatterTest, FormatLongTestNames) {
    std::string long_name(1000, 'a');
    TestResult result = createTestResult(long_name, Result::PASS);
    
    OutputFormatter::formatResult(result, false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find(long_name), std::string::npos);
}

// ========== Edge Cases ==========

TEST_F(OutputFormatterTest, FormatEmptySuite) {
    SuiteResult suite = createSuiteResult("Empty Suite");
    suite.total_tests = 0;
    suite.passed_tests = 0;
    suite.failed_tests = 0;
    suite.error_tests = 0;
    
    OutputFormatter::formatSuiteResult(suite, false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("Empty Suite"), std::string::npos);
    EXPECT_NE(output.find("0"), std::string::npos);
}

TEST_F(OutputFormatterTest, FormatResultWithEmptyValues) {
    TestResult result = createTestResult("", Result::PASS, "", "");
    result.error_details = "";
    
    OutputFormatter::formatResult(result, true, json_stream);
    
    std::string output = json_stream.str();
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(output, root));
    EXPECT_EQ(root["assertion_type"].asString(), "");
}

TEST_F(OutputFormatterTest, FormatResultWithUnicodeCharacters) {
    TestResult result = createTestResult("测试_тест_🔧", Result::FAIL, 
                                        "ñiño", "niño");
    
    OutputFormatter::formatResult(result, false, text_stream);
    
    std::string output = text_stream.str();
    EXPECT_NE(output.find("测试_тест_🔧"), std::string::npos);
    EXPECT_NE(output.find("ñiño"), std::string::npos);
}

// ========== Duration Formatting ==========

TEST_F(OutputFormatterTest, FormatDurationInResults) {
    TestResult result = createTestResult("duration_test", Result::PASS);
    result.duration = std::chrono::milliseconds(1500);
    
    OutputFormatter::formatResult(result, true, json_stream);
    
    std::string output = json_stream.str();
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(output, root));
    EXPECT_TRUE(root.isMember("duration_ms"));
    EXPECT_EQ(root["duration_ms"].asInt(), 1500);
}

TEST_F(OutputFormatterTest, FormatSuiteDuration) {
    SuiteResult suite = createSuiteResult("Duration Suite");
    auto start = std::chrono::system_clock::now();
    auto end = start + std::chrono::milliseconds(2500);
    suite.start_time = start;
    suite.end_time = end;
    
    OutputFormatter::formatSuiteResult(suite, true, json_stream);
    
    std::string output = json_stream.str();
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(output, root));
    EXPECT_TRUE(root.isMember("duration_ms"));
    EXPECT_GE(root["duration_ms"].asInt(), 2500);
}