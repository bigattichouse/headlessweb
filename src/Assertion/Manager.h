#pragma once

#include "Types.h"
#include "OutputFormatter.h"
#include "../Browser/Browser.h"
#include "../Session/Session.h"
#include <vector>
#include <memory>
#include <chrono>

namespace Assertion {

class Manager {
public:
    Manager();
    ~Manager();
    
    // Core assertion methods
    Result assertExists(Browser& browser, const Command& cmd);
    Result assertText(Browser& browser, const Command& cmd);
    Result assertElementValue(Browser& browser, const Command& cmd);
    Result assertCount(Browser& browser, const Command& cmd);
    Result assertJavaScript(Browser& browser, const Command& cmd);

    // Page-level assertion methods
    Result assertUrl(Browser& browser, const Command& cmd);
    Result assertTitle(Browser& browser, const Command& cmd);

    // Session header assertion methods
    Result assertResponseHeader(Session& session, const Command& cmd);
    Result assertRequestHeader(Session& session, const Command& cmd);
    Result assertStatusCode(Session& session, const Command& cmd);

    // Execute any assertion command
    Result executeAssertion(Browser& browser, const Command& cmd);
    // Session-aware overload (required for header/status assertions)
    Result executeAssertion(Browser& browser, Session& session, const Command& cmd);
    
    // Test suite management
    void startSuite(const std::string& name);
    void endSuite(bool json_output = false, const std::string& format = "text", bool suppress_exit = false);
    bool isSuiteActive() const;
    
    // Result tracking
    void addResult(const TestResult& result);
    const std::vector<TestResult>& getResults() const;
    void clearResults();
    
    // Statistics
    int getTotalTests() const;
    int getPassedTests() const;
    int getFailedTests() const;
    int getErrorTests() const;
    
    // Output control
    void setSilentMode(bool silent);
    void setJsonOutput(bool json);
    
private:
    std::vector<TestResult> results;
    std::unique_ptr<SuiteResult> current_suite;
    bool silent_mode;
    bool json_output;
    
    // Helper methods
    TestResult createResult(const Command& cmd, Result result, 
                          const std::string& actual = "", 
                          const std::string& error = "");
    
    bool compareValues(const std::string& actual, const std::string& expected, 
                      ComparisonOperator op, bool case_sensitive = true);
    
    ComparisonOperator parseOperator(const std::string& op_str);
    std::string extractOperatorFromValue(const std::string& value, ComparisonOperator& op);
    
    void outputResult(const TestResult& result);
};

} // namespace Assertion
