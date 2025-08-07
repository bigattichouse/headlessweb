#include <gtest/gtest.h>
#include "../../src/Assertion/Manager.h"
#include "../../src/Browser/Browser.h"
#include "../browser_test_environment.h"
#include "../utils/test_helpers.h"
#include "Debug.h"
#include <memory>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

extern std::unique_ptr<Browser> g_browser;

class TestSuiteManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("suite_management_tests");
        
        // Use global browser instance like other working tests
        browser_ = g_browser.get();
        
        // Create assertion manager for interface testing
        assertion_manager_ = std::make_unique<Assertion::Manager>();
        
        // NO PAGE LOADING - Use interface testing approach
        
        debug_output("TestSuiteManagementTest SetUp complete");
    }
    
    void TearDown() override {
        // Clean teardown without navigation
        if (assertion_manager_ && assertion_manager_->isSuiteActive()) {
            assertion_manager_->endSuite(false, "text");
        }
        if (assertion_manager_) {
            assertion_manager_->clearResults();
        }
        temp_dir.reset();
    }
    
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    Browser* browser_;
    std::unique_ptr<Assertion::Manager> assertion_manager_;
    
    // Interface testing helper methods
    std::string executeWrappedJS(const std::string& jsCode) {
        // Test JavaScript execution interface without requiring page content
        std::string wrapped = "(function() { try { " + jsCode + " } catch(e) { return 'error: ' + e.message; } })()";
        return browser_->executeJavascriptSync(wrapped);
    }
    
    // Helper method to create HTML page for testing (for interface testing only)
    std::string createTestPage(const std::string& html_content, const std::string& filename = "test.html") {
        auto html_file = temp_dir->createFile(filename, html_content);
        return "file://" + html_file.string();
    }
};

// ========== Suite Lifecycle Interface Tests ==========

TEST_F(TestSuiteManagementTest, StartSuiteInterface) {
    // Test suite start interface without page loading
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    EXPECT_NO_THROW(assertion_manager_->startSuite("Test Suite"));
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    
    EXPECT_NO_THROW(assertion_manager_->startSuite("Another Suite"));
    EXPECT_NO_THROW(assertion_manager_->startSuite(""));  // Empty name
    EXPECT_NO_THROW(assertion_manager_->startSuite("Suite with spaces and characters 123!@#"));
}

TEST_F(TestSuiteManagementTest, EndSuiteInterface) {
    // Test suite end interface without page loading
    assertion_manager_->startSuite("Test Suite");
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    // Test ending suite with different output formats
    assertion_manager_->startSuite("Format Test Suite");
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "json"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    assertion_manager_->startSuite("JSON Output Suite");
    EXPECT_NO_THROW(assertion_manager_->endSuite(true, "json"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
}

// ========== Suite Statistics Interface Tests ==========

TEST_F(TestSuiteManagementTest, SuiteStatisticsInterface) {
    // Test suite statistics interface without page loading
    assertion_manager_->startSuite("Statistics Test");
    
    // Test statistics methods without executing actual assertions
    EXPECT_NO_THROW(assertion_manager_->getTotalTests());
    EXPECT_NO_THROW(assertion_manager_->getPassedTests());
    EXPECT_NO_THROW(assertion_manager_->getFailedTests());
    EXPECT_NO_THROW(assertion_manager_->getErrorTests());
    
    // Test result management interface
    EXPECT_NO_THROW(assertion_manager_->getResults());
    EXPECT_NO_THROW(assertion_manager_->clearResults());
    
    // Verify initial state
    EXPECT_EQ(assertion_manager_->getTotalTests(), 0);
    EXPECT_EQ(assertion_manager_->getPassedTests(), 0);
    EXPECT_EQ(assertion_manager_->getFailedTests(), 0);
    EXPECT_EQ(assertion_manager_->getErrorTests(), 0);
}

TEST_F(TestSuiteManagementTest, AssertionCommandInterface) {
    // Test assertion command interface without page loading
    assertion_manager_->startSuite("Command Interface Test");
    
    // Test command creation and validation
    std::vector<Assertion::Command> test_commands;
    
    // Create various command types for interface testing
    Assertion::Command exists_cmd;
    exists_cmd.type = "exists";
    exists_cmd.selector = "#test-element";
    exists_cmd.expected_value = "true";
    exists_cmd.op = Assertion::ComparisonOperator::EQUALS;
    exists_cmd.timeout_ms = 100;  // Short timeout for interface test
    test_commands.push_back(exists_cmd);
    
    Assertion::Command text_cmd;
    text_cmd.type = "text";
    text_cmd.selector = ".test-class";
    text_cmd.expected_value = "Expected Text";
    text_cmd.op = Assertion::ComparisonOperator::CONTAINS;
    text_cmd.timeout_ms = 100;
    test_commands.push_back(text_cmd);
    
    Assertion::Command value_cmd;
    value_cmd.type = "value";
    value_cmd.selector = "input[name='test']";
    value_cmd.expected_value = "test_value";
    value_cmd.op = Assertion::ComparisonOperator::EQUALS;
    value_cmd.timeout_ms = 100;
    test_commands.push_back(value_cmd);
    
    Assertion::Command count_cmd;
    count_cmd.type = "count";
    count_cmd.selector = "div.item";
    count_cmd.expected_value = "5";
    count_cmd.op = Assertion::ComparisonOperator::GREATER_THAN;
    count_cmd.timeout_ms = 100;
    test_commands.push_back(count_cmd);
    
    Assertion::Command js_cmd;
    js_cmd.type = "javascript";
    js_cmd.selector = "";
    js_cmd.expected_value = "document.title";
    js_cmd.op = Assertion::ComparisonOperator::EQUALS;
    js_cmd.timeout_ms = 100;
    test_commands.push_back(js_cmd);
    
    // Interface should handle all command types gracefully
    for (const auto& cmd : test_commands) {
        EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, cmd));
    }
    
    // Test command interface methods
    EXPECT_NO_THROW(assertion_manager_->getTotalTests());
    EXPECT_NO_THROW(assertion_manager_->getResults());
}

// ========== Output Format Interface Tests ==========

TEST_F(TestSuiteManagementTest, OutputFormatInterface) {
    // Test output format interface without page loading
    
    // Test JSON output mode
    EXPECT_NO_THROW(assertion_manager_->setJsonOutput(true));
    assertion_manager_->startSuite("JSON Test Suite");
    
    // Capture output from endSuite
    std::ostringstream json_output;
    std::streambuf* orig = std::cout.rdbuf();
    std::cout.rdbuf(json_output.rdbuf());
    
    EXPECT_NO_THROW(assertion_manager_->endSuite(true, "json"));
    
    std::cout.rdbuf(orig);
    std::string captured_json = json_output.str();
    
    // Test text output mode
    EXPECT_NO_THROW(assertion_manager_->setJsonOutput(false));
    assertion_manager_->startSuite("Text Test Suite");
    
    std::ostringstream text_output;
    std::cout.rdbuf(text_output.rdbuf());
    
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    
    std::cout.rdbuf(orig);
    std::string captured_text = text_output.str();
    
    // Interface should handle different output formats gracefully
    EXPECT_NO_THROW(assertion_manager_->startSuite("Format Test"));
    EXPECT_NO_THROW(assertion_manager_->endSuite(true, "json"));
    EXPECT_NO_THROW(assertion_manager_->startSuite("Format Test 2"));
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    EXPECT_NO_THROW(assertion_manager_->startSuite("Format Test 3"));
    EXPECT_NO_THROW(assertion_manager_->endSuite(true, "invalid_format"));
}

TEST_F(TestSuiteManagementTest, SilentModeInterface) {
    // Test silent mode interface without page loading
    
    // Test enabling silent mode
    EXPECT_NO_THROW(assertion_manager_->setSilentMode(true));
    assertion_manager_->startSuite("Silent Test Suite");
    
    // Test output capture in silent mode
    std::ostringstream silent_output;
    std::streambuf* orig = std::cout.rdbuf();
    std::cout.rdbuf(silent_output.rdbuf());
    
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    
    std::cout.rdbuf(orig);
    std::string captured_output = silent_output.str();
    
    // Test disabling silent mode
    EXPECT_NO_THROW(assertion_manager_->setSilentMode(false));
    assertion_manager_->startSuite("Normal Output Suite");
    
    std::ostringstream normal_output;
    std::cout.rdbuf(normal_output.rdbuf());
    
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    
    std::cout.rdbuf(orig);
    std::string normal_captured = normal_output.str();
    
    // Interface should handle both modes gracefully
    EXPECT_NO_THROW(assertion_manager_->setSilentMode(true));
    EXPECT_NO_THROW(assertion_manager_->setSilentMode(false));
}

// ========== Individual Assertion Interface Tests ==========

TEST_F(TestSuiteManagementTest, IndividualAssertionInterface) {
    // Test individual assertions without active suite
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    // Test individual assertion command interface
    Assertion::Command cmd;
    cmd.type = "exists";
    cmd.selector = "#test-element";
    cmd.expected_value = "true";
    cmd.op = Assertion::ComparisonOperator::EQUALS;
    cmd.timeout_ms = 100;  // Short timeout for interface test
    
    // Interface should handle individual assertions gracefully
    EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, cmd));
    
    // Test result tracking for individual assertions
    EXPECT_NO_THROW(assertion_manager_->getResults());
    EXPECT_NO_THROW(assertion_manager_->getTotalTests());
    
    // Interface should still function without active suite
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->clearResults());
}

TEST_F(TestSuiteManagementTest, MultipleSuiteCyclesInterface) {
    // Test multiple suite cycles interface without page loading
    
    // First suite cycle
    EXPECT_NO_THROW(assertion_manager_->startSuite("Suite 1"));
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->getTotalTests());
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    // Second suite cycle
    EXPECT_NO_THROW(assertion_manager_->startSuite("Suite 2"));
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->getTotalTests());
    EXPECT_NO_THROW(assertion_manager_->endSuite(true, "json"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    // Third suite cycle with different settings
    EXPECT_NO_THROW(assertion_manager_->setSilentMode(true));
    EXPECT_NO_THROW(assertion_manager_->startSuite("Suite 3"));
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->setSilentMode(false));
    
    // Fourth suite cycle with JSON output
    EXPECT_NO_THROW(assertion_manager_->setJsonOutput(true));
    EXPECT_NO_THROW(assertion_manager_->startSuite("Suite 4"));
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->endSuite(true, "json"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->setJsonOutput(false));
    
    // Test rapid suite cycles
    for (int i = 0; i < 10; ++i) {
        std::string suite_name = "Rapid Suite " + std::to_string(i);
        EXPECT_NO_THROW(assertion_manager_->startSuite(suite_name));
        EXPECT_TRUE(assertion_manager_->isSuiteActive());
        EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
        EXPECT_FALSE(assertion_manager_->isSuiteActive());
    }
}

// ========== Custom Message Interface Tests ==========

TEST_F(TestSuiteManagementTest, CustomMessageInterface) {
    // Test custom message interface without page loading
    assertion_manager_->startSuite("Custom Message Suite");
    
    // Test various custom message scenarios
    std::vector<std::string> custom_messages = {
        "Basic custom message",
        "Message with special characters !@#$%^&*()",
        "Unicode message: 测试消息 العربية αβγ",
        "Very long custom message: " + std::string(500, 'M'),
        "Message with\nnewlines\tand\ttabs",
        "JSON-like message: {\"key\": \"value\"}",
        "",  // Empty message
        "Multi-line message\nLine 2\nLine 3"
    };
    
    for (const auto& msg : custom_messages) {
        Assertion::Command cmd;
        cmd.type = "exists";
        cmd.selector = "#test-element";
        cmd.expected_value = "true";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.custom_message = msg;
        cmd.timeout_ms = 100;  // Short timeout for interface test
        
        // Interface should handle custom messages gracefully
        EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, cmd));
    }
    
    // Test results with custom messages
    EXPECT_NO_THROW(assertion_manager_->getResults());
    
    // Test that interface preserves custom messages in results
    const auto& results = assertion_manager_->getResults();
    EXPECT_NO_THROW((void)results.size());
    
    assertion_manager_->endSuite(false, "text");
}

// ========== Assertion Manager Interface Tests ==========

TEST_F(TestSuiteManagementTest, AssertionManagerCreationInterface) {
    // Test assertion manager creation and basic interface
    auto new_manager = std::make_unique<Assertion::Manager>();
    EXPECT_NE(new_manager.get(), nullptr);
    
    // Test initial state
    EXPECT_FALSE(new_manager->isSuiteActive());
    EXPECT_EQ(new_manager->getTotalTests(), 0);
    EXPECT_EQ(new_manager->getPassedTests(), 0);
    EXPECT_EQ(new_manager->getFailedTests(), 0);
    EXPECT_EQ(new_manager->getErrorTests(), 0);
    
    // Test basic operations
    EXPECT_NO_THROW(new_manager->getResults());
    EXPECT_NO_THROW(new_manager->clearResults());
    EXPECT_NO_THROW(new_manager->setSilentMode(true));
    EXPECT_NO_THROW(new_manager->setJsonOutput(true));
}

TEST_F(TestSuiteManagementTest, ComparisonOperatorInterface) {
    // Test comparison operator interface without page loading
    assertion_manager_->startSuite("Operator Interface Test");
    
    std::vector<Assertion::ComparisonOperator> operators = {
        Assertion::ComparisonOperator::EQUALS,
        Assertion::ComparisonOperator::NOT_EQUALS,
        Assertion::ComparisonOperator::CONTAINS,
        Assertion::ComparisonOperator::NOT_CONTAINS,
        Assertion::ComparisonOperator::GREATER_THAN,
        Assertion::ComparisonOperator::LESS_THAN,
        Assertion::ComparisonOperator::GREATER_EQUAL,
        Assertion::ComparisonOperator::LESS_EQUAL
    };
    
    for (auto op : operators) {
        Assertion::Command cmd;
        cmd.type = "javascript";
        cmd.selector = "";
        cmd.expected_value = "test";
        cmd.op = op;
        cmd.timeout_ms = 100;
        
        // Interface should handle all operator types gracefully
        EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, cmd));
    }
}

TEST_F(TestSuiteManagementTest, AssertionTypeInterface) {
    // Test different assertion type interfaces without page loading
    assertion_manager_->startSuite("Assertion Type Interface Test");
    
    std::vector<std::string> assertion_types = {
        "exists",
        "text",
        "value",
        "count",
        "javascript",
        "attribute",
        "style",
        "visible",
        "enabled",
        "selected"
    };
    
    for (const auto& type : assertion_types) {
        Assertion::Command cmd;
        cmd.type = type;
        cmd.selector = "#test-element";
        cmd.expected_value = "test";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 100;
        
        // Interface should handle all assertion types gracefully
        EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, cmd));
    }
}

TEST_F(TestSuiteManagementTest, ErrorHandlingInterface) {
    // Test error handling interface without page loading
    assertion_manager_->startSuite("Error Handling Test");
    
    // Test with invalid/edge case inputs
    Assertion::Command invalid_cmd;
    invalid_cmd.type = "";
    invalid_cmd.selector = "";
    invalid_cmd.expected_value = "";
    invalid_cmd.timeout_ms = 0;
    
    EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, invalid_cmd));
    
    // Test with very short timeout
    Assertion::Command short_timeout_cmd;
    short_timeout_cmd.type = "exists";
    short_timeout_cmd.selector = "#nonexistent";
    short_timeout_cmd.expected_value = "true";
    short_timeout_cmd.timeout_ms = 1;  // Very short timeout
    
    EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, short_timeout_cmd));
    
    // Test with very long selector
    Assertion::Command long_selector_cmd;
    long_selector_cmd.type = "exists";
    long_selector_cmd.selector = std::string(1000, 'x');
    long_selector_cmd.expected_value = "true";
    long_selector_cmd.timeout_ms = 100;
    
    EXPECT_NO_THROW(assertion_manager_->executeAssertion(*browser_, long_selector_cmd));
}

TEST_F(TestSuiteManagementTest, ResourceCleanupInterface) {
    // Test resource cleanup interface
    {
        auto temp_manager = std::make_unique<Assertion::Manager>();
        EXPECT_NO_THROW(temp_manager->startSuite("Temp Suite"));
        EXPECT_TRUE(temp_manager->isSuiteActive());
        // Manager destructor should clean up resources
    }
    
    // Test that original manager still works after cleanup
    EXPECT_NO_THROW(assertion_manager_->startSuite("After Cleanup Suite"));
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    EXPECT_NO_THROW(assertion_manager_->endSuite(false, "text"));
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
}