#include <gtest/gtest.h>
#include "../../src/Assertion/Manager.h"
#include "../../src/Browser/Browser.h"
#include "../browser_test_environment.h"
#include "../utils/test_helpers.h"
#include <memory>
#include <sstream>
#include <iostream>

extern std::unique_ptr<Browser> g_browser;

class TestSuiteManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("suite_management_tests");
        browser_ = g_browser.get();
        assertion_manager_ = std::make_unique<Assertion::Manager>();
        
        // Create simple test HTML
        std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>Suite Test</title></head>
<body>
    <h1>Suite Test Page</h1>
    <div id="test1">Content 1</div>
    <div id="test2">Content 2</div>
</body>
</html>
)HTML";
        
        auto html_file = temp_dir->createFile("suite_test.html", test_html);
        test_url_ = "file://" + html_file.string();
        
        browser_->loadUri(test_url_);
        browser_->waitForNavigation(3000);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    void TearDown() override {
        // End any active suite
        if (assertion_manager_->isSuiteActive()) {
            assertion_manager_->endSuite(false, "text");
        }
        assertion_manager_->clearResults();
        browser_->loadUri("about:blank");
        browser_->waitForNavigation(1000);
    }
    
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    Browser* browser_;
    std::unique_ptr<Assertion::Manager> assertion_manager_;
    std::string test_url_;
};

// Test basic suite lifecycle
TEST_F(TestSuiteManagementTest, StartSuite_SetsActiveState) {
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    assertion_manager_->startSuite("Test Suite");
    
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
}

TEST_F(TestSuiteManagementTest, EndSuite_ClearsActiveState) {
    assertion_manager_->startSuite("Test Suite");
    EXPECT_TRUE(assertion_manager_->isSuiteActive());
    
    assertion_manager_->endSuite(false, "text");
    
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
}

// Test assertion tracking within suite
TEST_F(TestSuiteManagementTest, SuiteTracksAssertions_AllPass) {
    assertion_manager_->startSuite("Passing Tests");
    
    // Run multiple passing assertions
    Assertion::Command cmd1;
    cmd1.type = "exists";
    cmd1.selector = "h1";
    cmd1.expected_value = "true";
    cmd1.op = Assertion::ComparisonOperator::EQUALS;
    cmd1.timeout_ms = 5000;
    
    Assertion::Command cmd2;
    cmd2.type = "text";
    cmd2.selector = "#test1";
    cmd2.expected_value = "Content 1";
    cmd2.op = Assertion::ComparisonOperator::EQUALS;
    cmd2.timeout_ms = 5000;
    
    Assertion::Result result1 = assertion_manager_->executeAssertion(*browser_, cmd1);
    Assertion::Result result2 = assertion_manager_->executeAssertion(*browser_, cmd2);
    
    EXPECT_EQ(result1, Assertion::Result::PASS);
    EXPECT_EQ(result2, Assertion::Result::PASS);
    
    // Check statistics
    EXPECT_EQ(assertion_manager_->getTotalTests(), 2);
    EXPECT_EQ(assertion_manager_->getPassedTests(), 2);
    EXPECT_EQ(assertion_manager_->getFailedTests(), 0);
    EXPECT_EQ(assertion_manager_->getErrorTests(), 0);
}

TEST_F(TestSuiteManagementTest, SuiteTracksAssertions_MixedResults) {
    assertion_manager_->startSuite("Mixed Results");
    
    // Passing assertion
    Assertion::Command cmd1;
    cmd1.type = "exists";
    cmd1.selector = "h1";
    cmd1.expected_value = "true";
    cmd1.op = Assertion::ComparisonOperator::EQUALS;
    cmd1.timeout_ms = 5000;
    
    // Failing assertion
    Assertion::Command cmd2;
    cmd2.type = "exists";
    cmd2.selector = "#nonexistent";
    cmd2.expected_value = "true";
    cmd2.op = Assertion::ComparisonOperator::EQUALS;
    cmd2.timeout_ms = 1000;
    
    Assertion::Result result1 = assertion_manager_->executeAssertion(*browser_, cmd1);
    Assertion::Result result2 = assertion_manager_->executeAssertion(*browser_, cmd2);
    
    EXPECT_EQ(result1, Assertion::Result::PASS);
    EXPECT_EQ(result2, Assertion::Result::FAIL);
    
    // Check statistics
    EXPECT_EQ(assertion_manager_->getTotalTests(), 2);
    EXPECT_EQ(assertion_manager_->getPassedTests(), 1);
    EXPECT_EQ(assertion_manager_->getFailedTests(), 1);
    EXPECT_EQ(assertion_manager_->getErrorTests(), 0);
}

// Test JSON output mode
TEST_F(TestSuiteManagementTest, SuiteJSONOutput_ProducesValidStructure) {
    assertion_manager_->setJsonOutput(true);
    assertion_manager_->startSuite("JSON Test Suite");
    
    // Run a simple assertion
    Assertion::Command cmd;
    cmd.type = "exists";
    cmd.selector = "h1";
    cmd.expected_value = "true";
    cmd.op = Assertion::ComparisonOperator::EQUALS;
    cmd.timeout_ms = 5000;
    cmd.json_output = true;
    
    Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
    
    // Capture output from endSuite
    std::ostringstream output;
    std::streambuf* orig = std::cout.rdbuf();
    std::cout.rdbuf(output.rdbuf());
    
    assertion_manager_->endSuite(true, "json");
    
    std::cout.rdbuf(orig);
    std::string json_output = output.str();
    
    // Basic JSON structure validation
    EXPECT_TRUE(json_output.find("\"suite\":") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"total\":") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"passed\":") != std::string::npos);
    EXPECT_TRUE(json_output.find("\"failed\":") != std::string::npos);
}

// Test silent mode
TEST_F(TestSuiteManagementTest, SuiteSilentMode_NoOutput) {
    assertion_manager_->setSilentMode(true);
    assertion_manager_->startSuite("Silent Test Suite");
    
    // Run assertion
    Assertion::Command cmd;
    cmd.type = "exists";
    cmd.selector = "h1";
    cmd.expected_value = "true";
    cmd.op = Assertion::ComparisonOperator::EQUALS;
    cmd.timeout_ms = 5000;
    cmd.silent = true;
    
    // Capture output
    std::ostringstream output;
    std::streambuf* orig = std::cout.rdbuf();
    std::cout.rdbuf(output.rdbuf());
    
    Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
    
    std::cout.rdbuf(orig);
    std::string captured_output = output.str();
    
    EXPECT_EQ(result, Assertion::Result::PASS);
    // In silent mode, there should be minimal or no output
    EXPECT_TRUE(captured_output.empty() || captured_output.find("PASS") == std::string::npos);
}

// Test suite without active suite (individual assertions)
TEST_F(TestSuiteManagementTest, IndividualAssertions_WithoutSuite) {
    // Don't start a suite
    EXPECT_FALSE(assertion_manager_->isSuiteActive());
    
    // Run assertion
    Assertion::Command cmd;
    cmd.type = "exists";
    cmd.selector = "h1";
    cmd.expected_value = "true";
    cmd.op = Assertion::ComparisonOperator::EQUALS;
    cmd.timeout_ms = 5000;
    
    Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
    
    // Results should still be tracked
    EXPECT_GT(assertion_manager_->getResults().size(), 0);
}

// Test multiple suite cycles
TEST_F(TestSuiteManagementTest, MultipleSuiteCycles_IndependentResults) {
    // First suite
    assertion_manager_->startSuite("Suite 1");
    
    Assertion::Command cmd1;
    cmd1.type = "exists";
    cmd1.selector = "h1";
    cmd1.expected_value = "true";
    cmd1.op = Assertion::ComparisonOperator::EQUALS;
    cmd1.timeout_ms = 5000;
    
    assertion_manager_->executeAssertion(*browser_, cmd1);
    EXPECT_EQ(assertion_manager_->getTotalTests(), 1);
    
    assertion_manager_->endSuite(false, "text");
    
    // Second suite
    assertion_manager_->startSuite("Suite 2");
    
    Assertion::Command cmd2;
    cmd2.type = "text";
    cmd2.selector = "#test1";
    cmd2.expected_value = "Content 1";
    cmd2.op = Assertion::ComparisonOperator::EQUALS;
    cmd2.timeout_ms = 5000;
    
    Assertion::Command cmd3;
    cmd3.type = "text";
    cmd3.selector = "#test2";
    cmd3.expected_value = "Content 2";
    cmd3.op = Assertion::ComparisonOperator::EQUALS;
    cmd3.timeout_ms = 5000;
    
    assertion_manager_->executeAssertion(*browser_, cmd2);
    assertion_manager_->executeAssertion(*browser_, cmd3);
    
    EXPECT_EQ(assertion_manager_->getTotalTests(), 2); // Should reset for new suite
    EXPECT_EQ(assertion_manager_->getPassedTests(), 2);
    
    assertion_manager_->endSuite(false, "text");
}

// Test custom messages in suite context
TEST_F(TestSuiteManagementTest, SuiteWithCustomMessages_PreservesMessages) {
    assertion_manager_->startSuite("Custom Message Suite");
    
    Assertion::Command cmd;
    cmd.type = "exists";
    cmd.selector = "h1";
    cmd.expected_value = "true";
    cmd.op = Assertion::ComparisonOperator::EQUALS;
    cmd.custom_message = "Page title should be present";
    cmd.timeout_ms = 5000;
    
    Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
    
    const auto& results = assertion_manager_->getResults();
    EXPECT_FALSE(results.empty());
    if (!results.empty()) {
        EXPECT_EQ(results.back().message, "Page title should be present");
    }
}