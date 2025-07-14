#include "OutputFormatter.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace Assertion {

void OutputFormatter::formatResult(const TestResult& result, bool json_mode, std::ostream& out) {
    if (json_mode) {
        Json::Value json = testResultToJson(result);
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        out << Json::writeString(builder, json) << std::endl;
    } else {
        // Text output
        std::string status = resultToString(result.result);
        std::string duration_str = formatDuration(result.duration);
        
        if (result.result == Result::PASS) {
            out << "PASS: " << result.assertion_type;
        } else if (result.result == Result::FAIL) {
            out << "FAIL: " << result.assertion_type;
        } else {
            out << "ERROR: " << result.assertion_type;
        }
        
        if (!result.selector.empty()) {
            out << " (" << result.selector << ")";
        }
        
        if (!result.message.empty()) {
            out << " - " << result.message;
        } else if (result.result != Result::PASS) {
            out << " - Expected: " << result.expected;
            if (!result.actual.empty()) {
                out << ", Actual: " << result.actual;
            }
        }
        
        out << " [" << duration_str << "]";
        
        if (!result.error_details.empty()) {
            out << " (" << result.error_details << ")";
        }
        
        out << std::endl;
    }
}

void OutputFormatter::formatError(const std::string& command, const std::string& error, bool json_mode, std::ostream& out) {
    if (json_mode) {
        Json::Value json;
        json["command"] = command;
        json["success"] = false;
        json["error"] = error;
        json["result"] = "ERROR";
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        out << Json::writeString(builder, json) << std::endl;
    } else {
        out << "ERROR: " << command << " - " << error << std::endl;
    }
}

void OutputFormatter::formatSuiteResult(const SuiteResult& suite, bool json_mode, std::ostream& out) {
    if (json_mode) {
        Json::Value json = suiteResultToJson(suite);
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  "; // Pretty print for suite results
        out << Json::writeString(builder, json) << std::endl;
    } else {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            suite.end_time - suite.start_time);
        
        out << "\n=== Test Suite Results ===" << std::endl;
        out << "Suite: " << suite.suite_name << std::endl;
        out << "Total: " << suite.total_tests 
            << ", Passed: " << suite.passed_tests 
            << ", Failed: " << suite.failed_tests;
        
        if (suite.error_tests > 0) {
            out << ", Errors: " << suite.error_tests;
        }
        
        out << std::endl;
        out << "Duration: " << formatDuration(duration) << std::endl;
        
        if (suite.failed_tests > 0 || suite.error_tests > 0) {
            out << "\nFailed/Error Tests:" << std::endl;
            for (const auto& result : suite.test_results) {
                if (result.result != Result::PASS) {
                    out << "  - " << result.assertion_type;
                    if (!result.selector.empty()) {
                        out << " (" << result.selector << ")";
                    }
                    if (!result.error_details.empty()) {
                        out << ": " << result.error_details;
                    }
                    out << std::endl;
                }
            }
        }
    }
}

void OutputFormatter::formatJUnitXML(const SuiteResult& suite, std::ostream& out) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        suite.end_time - suite.start_time);
    
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    out << "<testsuites>" << std::endl;
    out << "  <testsuite name=\"" << escapeXML(suite.suite_name) << "\""
        << " tests=\"" << suite.total_tests << "\""
        << " failures=\"" << suite.failed_tests << "\""
        << " errors=\"" << suite.error_tests << "\""
        << " time=\"" << std::fixed << std::setprecision(3) << (duration.count() / 1000.0) << "\">" << std::endl;
    
    for (const auto& result : suite.test_results) {
        std::string test_name = result.assertion_type;
        if (!result.selector.empty()) {
            test_name += " (" + result.selector + ")";
        }
        
        out << "    <testcase name=\"" << escapeXML(test_name) << "\""
            << " time=\"" << std::fixed << std::setprecision(3) << (result.duration.count() / 1000.0) << "\"";
        
        if (result.result == Result::PASS) {
            out << "/>" << std::endl;
        } else {
            out << ">" << std::endl;
            
            if (result.result == Result::FAIL) {
                out << "      <failure message=\"" << escapeXML(result.message.empty() ? "Assertion failed" : result.message) << "\">";
                out << escapeXML("Expected: " + result.expected + ", Actual: " + result.actual);
                out << "</failure>" << std::endl;
            } else {
                out << "      <error message=\"" << escapeXML(result.error_details) << "\">";
                out << escapeXML(result.error_details);
                out << "</error>" << std::endl;
            }
            
            out << "    </testcase>" << std::endl;
        }
    }
    
    out << "  </testsuite>" << std::endl;
    out << "</testsuites>" << std::endl;
}

Json::Value OutputFormatter::testResultToJson(const TestResult& result) {
    Json::Value json;
    json["assertion"] = result.assertion_type;
    json["selector"] = result.selector;
    json["result"] = resultToString(result.result);
    json["expected"] = result.expected;
    json["actual"] = result.actual;
    json["duration_ms"] = static_cast<int>(result.duration.count());
    
    if (!result.message.empty()) {
        json["message"] = result.message;
    }
    
    if (!result.error_details.empty()) {
        json["error"] = result.error_details;
    }
    
    return json;
}

Json::Value OutputFormatter::suiteResultToJson(const SuiteResult& suite) {
    Json::Value json;
    json["suite"] = suite.suite_name;
    json["total"] = suite.total_tests;
    json["passed"] = suite.passed_tests;
    json["failed"] = suite.failed_tests;
    json["errors"] = suite.error_tests;
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        suite.end_time - suite.start_time);
    json["duration_ms"] = static_cast<int>(duration.count());
    
    Json::Value tests(Json::arrayValue);
    for (const auto& result : suite.test_results) {
        tests.append(testResultToJson(result));
    }
    json["tests"] = tests;
    
    return json;
}

std::string OutputFormatter::resultToString(Result result) {
    switch (result) {
        case Result::PASS: return "PASS";
        case Result::FAIL: return "FAIL";
        case Result::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string OutputFormatter::formatDuration(std::chrono::milliseconds duration) {
    auto count = duration.count();
    if (count < 1000) {
        return std::to_string(count) + "ms";
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (count / 1000.0) << "s";
        return oss.str();
    }
}

std::string OutputFormatter::escapeXML(const std::string& text) {
    std::string escaped = text;
    
    // Replace in order: & first, then < and >
    size_t pos = 0;
    while ((pos = escaped.find("&", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&amp;");
        pos += 5;
    }
    
    pos = 0;
    while ((pos = escaped.find("<", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&lt;");
        pos += 4;
    }
    
    pos = 0;
    while ((pos = escaped.find(">", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&gt;");
        pos += 4;
    }
    
    pos = 0;
    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&quot;");
        pos += 6;
    }
    
    pos = 0;
    while ((pos = escaped.find("'", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&apos;");
        pos += 6;
    }
    
    return escaped;
}

} // namespace Assertion
