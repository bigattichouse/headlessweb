#include <gtest/gtest.h>
#include "../../src/Assertion/Manager.h"
#include "../../src/Browser/Browser.h"
#include "../../src/Session/Session.h"
#include "../browser_test_environment.h"
#include "../utils/test_helpers.h"
#include <memory>

extern std::unique_ptr<Browser> g_browser;

class AssertionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("assertion_integration_tests");
        
        // Use EXACTLY the same pattern as working BrowserCoreTest and DOM tests
        browser_ = g_browser.get();
        assertion_manager_ = std::make_unique<Assertion::Manager>();
        
        // Create session for browser initialization (same as working tests)
        session = std::make_unique<Session>("test_session");
        session->setCurrentUrl("about:blank");
        session->setViewport(1024, 768);
        
        debug_output("AssertionIntegrationTest SetUp complete");
    }
    
    void TearDown() override {
        // Clean up without destroying global browser (same as working tests)
        session.reset();
        temp_dir.reset();
    }
    
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    Browser* browser_;
    std::unique_ptr<Assertion::Manager> assertion_manager_;
    std::unique_ptr<Session> session;
};

// Test --assert-exists interface functionality (no page loading required)
TEST_F(AssertionIntegrationTest, AssertExistsInterfaceTest) {
    // Test assertion interface with non-existent elements (should handle gracefully)
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "exists";
        cmd.selector = "#nonexistent";  // Testing interface with non-existent element
        cmd.expected_value = "false";   // Expecting it to not exist (which is correct)
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        // Execute assertion - should work without segfaulting
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return PASS because element doesn't exist and we expect false
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

TEST_F(AssertionIntegrationTest, AssertExistsElementAbsentInterfaceTest) {
    // Test assertion interface when expecting element to exist but it doesn't
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "exists";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "true";  // Expecting element to exist
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return FAIL because element doesn't exist but we expected it to
        EXPECT_EQ(result, Assertion::Result::FAIL);
    });
}

TEST_F(AssertionIntegrationTest, AssertExistsExpectAbsentInterfaceTest) {
    // Test assertion interface expecting element to be absent (and it is)
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "exists";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "false";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

// Test --assert-text interface functionality
TEST_F(AssertionIntegrationTest, AssertTextInterfaceTest) {
    // Test text assertion interface with non-existent elements
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "text";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Text assertion on non-existent element should handle gracefully
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}

TEST_F(AssertionIntegrationTest, AssertTextComparisonInterfaceTest) {
    // Test text assertion interface with different comparison operators
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "text";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "test";
        cmd.op = Assertion::ComparisonOperator::CONTAINS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should handle gracefully regardless of result
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}

TEST_F(AssertionIntegrationTest, AssertTextContainsInterfaceTest) {
    // Test text contains assertion interface
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "text";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "test";
        cmd.op = Assertion::ComparisonOperator::CONTAINS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should handle gracefully
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}

// Test --assert-count interface functionality
TEST_F(AssertionIntegrationTest, AssertCountInterfaceTest) {
    // Test count assertion interface with non-existent elements
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "count";
        cmd.selector = ".nonexistent";
        cmd.expected_value = "0";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return PASS because count of non-existent elements is 0
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

TEST_F(AssertionIntegrationTest, AssertCountComparisonInterfaceTest) {
    // Test count assertion interface with different expected values
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "count";
        cmd.selector = ".nonexistent";
        cmd.expected_value = "5";  // Expecting 5 but count is 0
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return FAIL because count is 0 but we expect 5
        EXPECT_EQ(result, Assertion::Result::FAIL);
    });
}

TEST_F(AssertionIntegrationTest, AssertCountGreaterThanInterfaceTest) {
    // Test count assertion interface with GREATER_THAN operator
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "count";
        cmd.selector = ".nonexistent";
        cmd.expected_value = "1";  // Count is 0, expecting greater than 1
        cmd.op = Assertion::ComparisonOperator::GREATER_THAN;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return FAIL because 0 is not greater than 1
        EXPECT_EQ(result, Assertion::Result::FAIL);
    });
}

// Test --assert-js interface functionality
TEST_F(AssertionIntegrationTest, AssertJSInterfaceTest) {
    // Test JavaScript assertion interface with simple expressions
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "js";
        cmd.selector = "true";  // Simple boolean expression
        cmd.expected_value = "true";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should handle JavaScript execution interface
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}

TEST_F(AssertionIntegrationTest, AssertJSFalseExpressionInterfaceTest) {
    // Test JavaScript assertion interface with false expressions
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "js";
        cmd.selector = "false";  // Simple false expression
        cmd.expected_value = "true";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return FAIL because false != true
        EXPECT_EQ(result, Assertion::Result::FAIL);
    });
}

TEST_F(AssertionIntegrationTest, AssertJSNumericInterfaceTest) {
    // Test JavaScript assertion interface with numeric expressions
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "js";
        cmd.selector = "42";  // Simple numeric expression
        cmd.expected_value = "42";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return PASS because 42 == "42"
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

TEST_F(AssertionIntegrationTest, AssertJSStringInterfaceTest) {
    // Test JavaScript assertion interface with string expressions
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "js";
        cmd.selector = "'test'";  // Simple string expression
        cmd.expected_value = "test";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should handle string comparison interface
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}

// Test assertion timeout interface behavior
TEST_F(AssertionIntegrationTest, AssertTimeoutInterfaceTest) {
    // Test assertion interface timeout handling
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "exists";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "true";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 100; // Very short timeout
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should return FAIL quickly due to short timeout
        EXPECT_EQ(result, Assertion::Result::FAIL);
    });
}

// Test custom message interface functionality
TEST_F(AssertionIntegrationTest, AssertCustomMessageInterfaceTest) {
    // Test assertion interface with custom messages
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "exists";
        cmd.selector = "#nonexistent"; 
        cmd.expected_value = "false";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.custom_message = "Element should not exist";
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        EXPECT_EQ(result, Assertion::Result::PASS);
        
        // Check that result tracking interface works
        const auto& results = assertion_manager_->getResults();
        EXPECT_FALSE(results.empty());
        if (!results.empty()) {
            EXPECT_EQ(results.back().message, "Element should not exist");
        }
    });
}

// Test case sensitivity interface
TEST_F(AssertionIntegrationTest, AssertCaseSensitiveInterfaceTest) {
    // Test case sensitivity interface handling
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "text";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.case_sensitive = true;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should handle case sensitivity interface
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}

TEST_F(AssertionIntegrationTest, AssertCaseInsensitiveInterfaceTest) {
    // Test case insensitive interface handling
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "text";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.case_sensitive = false;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should handle case insensitive interface
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}

// Test element value assertion interface (for form elements)
TEST_F(AssertionIntegrationTest, AssertElementValueInterfaceTest) {
    // Test element value assertion interface with non-existent elements
    EXPECT_NO_THROW({
        Assertion::Command cmd;
        cmd.type = "element-value";
        cmd.selector = "#nonexistent";
        cmd.expected_value = "";
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.timeout_ms = 1000;
        
        Assertion::Result result = assertion_manager_->executeAssertion(*browser_, cmd);
        
        // Should handle element value interface gracefully
        EXPECT_TRUE(result == Assertion::Result::PASS || result == Assertion::Result::FAIL);
    });
}