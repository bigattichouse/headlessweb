#pragma once

#include "Types.h"
#include <json/json.h>
#include <string>
#include <iostream>

namespace Assertion {

class OutputFormatter {
public:
    // Format single test result
    static void formatResult(const TestResult& result, bool json_mode = false, std::ostream& out = std::cout);
    
    // Format error message
    static void formatError(const std::string& command, const std::string& error, bool json_mode = false, std::ostream& out = std::cerr);
    
    // Format test suite results
    static void formatSuiteResult(const SuiteResult& suite, bool json_mode = false, std::ostream& out = std::cout);
    
    // Format results in JUnit XML format
    static void formatJUnitXML(const SuiteResult& suite, std::ostream& out = std::cout);
    
private:
    static Json::Value testResultToJson(const TestResult& result);
    static Json::Value suiteResultToJson(const SuiteResult& suite);
    static std::string resultToString(Result result);
    static std::string formatDuration(std::chrono::milliseconds duration);
    static std::string escapeXML(const std::string& text);
};

} // namespace Assertion
