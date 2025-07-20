#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Assertion/Manager.h"
#include "Assertion/Types.h"
#include "../mocks/mock_browser.h"

using namespace Assertion;
using ::testing::Return;
using ::testing::_;

class AssertionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<Manager>();
        mockBrowser = std::make_unique<MockBrowser>();
    }

    void TearDown() override {
        manager.reset();
        mockBrowser.reset();
    }

    std::unique_ptr<Manager> manager;
    std::unique_ptr<MockBrowser> mockBrowser;
    
    Command createCommand(const std::string& type, 
                         const std::string& selector = "",
                         const std::string& expected = "",
                         ComparisonOperator op = ComparisonOperator::EQUALS) {
        Command cmd;
        cmd.type = type;
        cmd.selector = selector;
        cmd.expected_value = expected;
        cmd.op = op;
        cmd.json_output = false;
        cmd.silent = false;
        cmd.case_sensitive = true;
        cmd.timeout_ms = 5000;
        return cmd;
    }
};

// ========== Constructor and Basic Setup ==========

TEST_F(AssertionManagerTest, ConstructorInitializesCorrectly) {
    EXPECT_EQ(manager->getTotalTests(), 0);
    EXPECT_EQ(manager->getPassedTests(), 0);
    EXPECT_EQ(manager->getFailedTests(), 0);
    EXPECT_EQ(manager->getErrorTests(), 0);
    EXPECT_FALSE(manager->isSuiteActive());
    EXPECT_TRUE(manager->getResults().empty());
}

// ========== Assert Exists Tests ==========

TEST_F(AssertionManagerTest, AssertExistsPassWhenElementExists) {
    Command cmd = createCommand("exists", "#test-element");
    
    // Mock: element exists
    EXPECT_CALL(*mockBrowser, elementExists("#test-element"))
        .WillOnce(Return(true));
    
    Result result = manager->assertExists(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertExistsFailWhenElementDoesNotExist) {
    Command cmd = createCommand("exists", "#missing-element");
    
    // Mock: element does not exist
    EXPECT_CALL(*mockBrowser, elementExists("#missing-element"))
        .WillOnce(Return(false));
    
    Result result = manager->assertExists(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::FAIL);
}

TEST_F(AssertionManagerTest, AssertExistsWithInvalidSelector) {
    Command cmd = createCommand("exists", "invalid>>selector");
    
    // Mock: browser throws exception for invalid selector
    EXPECT_CALL(*mockBrowser, elementExists("invalid>>selector"))
        .WillOnce(testing::Throw(std::runtime_error("Invalid selector")));
    
    Result result = manager->assertExists(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::ERROR);
}

// ========== Assert Text Tests ==========

TEST_F(AssertionManagerTest, AssertTextPassWithExactMatch) {
    Command cmd = createCommand("text", "#content", "Expected Text");
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content"))
        .WillOnce(Return("Expected Text"));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertTextFailWithMismatch) {
    Command cmd = createCommand("text", "#content", "Expected Text");
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content"))
        .WillOnce(Return("Actual Text"));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::FAIL);
}

TEST_F(AssertionManagerTest, AssertTextWithContainsOperator) {
    Command cmd = createCommand("text", "#content", "partial", ComparisonOperator::CONTAINS);
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content"))
        .WillOnce(Return("This contains partial text"));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertTextWithNotContainsOperator) {
    Command cmd = createCommand("text", "#content", "missing", ComparisonOperator::NOT_CONTAINS);
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content"))
        .WillOnce(Return("This text does not have the word"));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertTextCaseInsensitive) {
    Command cmd = createCommand("text", "#content", "EXPECTED");
    cmd.case_sensitive = false;
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content"))
        .WillOnce(Return("expected"));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertTextWithRegexMatch) {
    Command cmd = createCommand("text", "#content", "\\d{4}-\\d{2}-\\d{2}", ComparisonOperator::REGEX_MATCH);
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content"))
        .WillOnce(Return("Today is 2024-01-15"));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertTextElementNotFound) {
    Command cmd = createCommand("text", "#missing", "Any Text");
    
    EXPECT_CALL(*mockBrowser, getInnerText("#missing"))
        .WillOnce(testing::Throw(std::runtime_error("Element not found")));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::ERROR);
}

// ========== Assert Count Tests ==========

TEST_F(AssertionManagerTest, AssertCountExactMatch) {
    Command cmd = createCommand("count", ".list-item", "5");
    
    EXPECT_CALL(*mockBrowser, countElements(".list-item"))
        .WillOnce(Return(5));
    
    Result result = manager->assertCount(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertCountMismatch) {
    Command cmd = createCommand("count", ".list-item", "3");
    
    EXPECT_CALL(*mockBrowser, countElements(".list-item"))
        .WillOnce(Return(7));
    
    Result result = manager->assertCount(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::FAIL);
}

TEST_F(AssertionManagerTest, AssertCountGreaterThan) {
    Command cmd = createCommand("count", ".item", "10", ComparisonOperator::GREATER_THAN);
    
    EXPECT_CALL(*mockBrowser, countElements(".item"))
        .WillOnce(Return(15));
    
    Result result = manager->assertCount(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertCountLessThanOrEqual) {
    Command cmd = createCommand("count", ".item", "20", ComparisonOperator::LESS_EQUAL);
    
    EXPECT_CALL(*mockBrowser, countElements(".item"))
        .WillOnce(Return(20));
    
    Result result = manager->assertCount(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertCountZeroElements) {
    Command cmd = createCommand("count", ".nonexistent", "0");
    
    EXPECT_CALL(*mockBrowser, countElements(".nonexistent"))
        .WillOnce(Return(0));
    
    Result result = manager->assertCount(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

// ========== Assert JavaScript Tests ==========

TEST_F(AssertionManagerTest, AssertJavaScriptTrueCondition) {
    Command cmd = createCommand("js", "", "document.title === 'Test Page'");
    
    EXPECT_CALL(*mockBrowser, executeJavascriptSync("document.title === 'Test Page'"))
        .WillOnce(Return("true"));
    
    Result result = manager->assertJavaScript(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertJavaScriptFalseCondition) {
    Command cmd = createCommand("js", "", "window.nonexistentProperty === true");
    
    EXPECT_CALL(*mockBrowser, executeJavascriptSync("window.nonexistentProperty === true"))
        .WillOnce(Return("false"));
    
    Result result = manager->assertJavaScript(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::FAIL);
}

TEST_F(AssertionManagerTest, AssertJavaScriptWithExpectedValue) {
    Command cmd = createCommand("js", "", "document.querySelectorAll('.item').length", "5");
    
    EXPECT_CALL(*mockBrowser, executeJavascriptSync("document.querySelectorAll('.item').length"))
        .WillOnce(Return("5"));
    
    Result result = manager->assertJavaScript(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertJavaScriptStringComparison) {
    Command cmd = createCommand("js", "", "document.title", "My Page Title");
    
    EXPECT_CALL(*mockBrowser, executeJavascriptSync("document.title"))
        .WillOnce(Return("My Page Title"));
    
    Result result = manager->assertJavaScript(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, AssertJavaScriptError) {
    Command cmd = createCommand("js", "", "invalid.javascript.syntax");
    
    EXPECT_CALL(*mockBrowser, executeJavascriptSync("invalid.javascript.syntax"))
        .WillOnce(testing::Throw(std::runtime_error("JavaScript execution error")));
    
    Result result = manager->assertJavaScript(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::ERROR);
}

// ========== Execute Assertion Tests ==========

TEST_F(AssertionManagerTest, ExecuteAssertionDispatchesToCorrectMethod) {
    // Test exists
    Command existsCmd = createCommand("exists", "#test");
    EXPECT_CALL(*mockBrowser, elementExists("#test")).WillOnce(Return(true));
    Result result1 = manager->executeAssertion(*mockBrowser, existsCmd);
    EXPECT_EQ(result1, Result::PASS);
    
    // Test text
    Command textCmd = createCommand("text", "#content", "test");
    EXPECT_CALL(*mockBrowser, getInnerText("#content")).WillOnce(Return("test"));
    Result result2 = manager->executeAssertion(*mockBrowser, textCmd);
    EXPECT_EQ(result2, Result::PASS);
    
    // Test count
    Command countCmd = createCommand("count", ".item", "3");
    EXPECT_CALL(*mockBrowser, countElements(".item")).WillOnce(Return(3));
    Result result3 = manager->executeAssertion(*mockBrowser, countCmd);
    EXPECT_EQ(result3, Result::PASS);
    
    // Test js
    Command jsCmd = createCommand("js", "", "true");
    EXPECT_CALL(*mockBrowser, executeJavascriptSync("true")).WillOnce(Return("true"));
    Result result4 = manager->executeAssertion(*mockBrowser, jsCmd);
    EXPECT_EQ(result4, Result::PASS);
}

TEST_F(AssertionManagerTest, ExecuteAssertionWithUnknownType) {
    Command cmd = createCommand("unknown_type", "#test");
    
    Result result = manager->executeAssertion(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::ERROR);
}

// ========== Test Suite Management ==========

TEST_F(AssertionManagerTest, TestSuiteLifecycle) {
    EXPECT_FALSE(manager->isSuiteActive());
    
    manager->startSuite("Test Suite");
    EXPECT_TRUE(manager->isSuiteActive());
    
    manager->endSuite();
    EXPECT_FALSE(manager->isSuiteActive());
}

TEST_F(AssertionManagerTest, TestSuiteAccumulatesResults) {
    manager->startSuite("Accumulation Test");
    
    // Add some test results
    Command cmd1 = createCommand("exists", "#pass");
    EXPECT_CALL(*mockBrowser, elementExists("#pass")).WillOnce(Return(true));
    manager->executeAssertion(*mockBrowser, cmd1);
    
    Command cmd2 = createCommand("exists", "#fail");
    EXPECT_CALL(*mockBrowser, elementExists("#fail")).WillOnce(Return(false));
    manager->executeAssertion(*mockBrowser, cmd2);
    
    Command cmd3 = createCommand("exists", "invalid>>selector");
    EXPECT_CALL(*mockBrowser, elementExists("invalid>>selector"))
        .WillOnce(testing::Throw(std::runtime_error("Error")));
    manager->executeAssertion(*mockBrowser, cmd3);
    
    EXPECT_EQ(manager->getTotalTests(), 3);
    EXPECT_EQ(manager->getPassedTests(), 1);
    EXPECT_EQ(manager->getFailedTests(), 1);
    EXPECT_EQ(manager->getErrorTests(), 1);
    
    manager->endSuite();
}

TEST_F(AssertionManagerTest, NestedSuitesNotAllowed) {
    manager->startSuite("Suite 1");
    EXPECT_TRUE(manager->isSuiteActive());
    
    // Starting another suite should end the first one
    manager->startSuite("Suite 2");
    EXPECT_TRUE(manager->isSuiteActive());
    
    manager->endSuite();
    EXPECT_FALSE(manager->isSuiteActive());
}

// ========== Result Tracking ==========

TEST_F(AssertionManagerTest, ResultTrackingAndRetrieval) {
    Command cmd = createCommand("exists", "#test");
    EXPECT_CALL(*mockBrowser, elementExists("#test")).WillOnce(Return(true));
    
    manager->executeAssertion(*mockBrowser, cmd);
    
    const auto& results = manager->getResults();
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].assertion_type, "exists");
    EXPECT_EQ(results[0].selector, "#test");
    EXPECT_EQ(results[0].result, Result::PASS);
}

TEST_F(AssertionManagerTest, ClearResults) {
    Command cmd = createCommand("exists", "#test");
    EXPECT_CALL(*mockBrowser, elementExists("#test")).WillOnce(Return(true));
    
    manager->executeAssertion(*mockBrowser, cmd);
    EXPECT_EQ(manager->getResults().size(), 1);
    
    manager->clearResults();
    EXPECT_TRUE(manager->getResults().empty());
    EXPECT_EQ(manager->getTotalTests(), 0);
}

TEST_F(AssertionManagerTest, StatisticsAccuracy) {
    // Execute various assertions
    Command pass1 = createCommand("exists", "#pass1");
    EXPECT_CALL(*mockBrowser, elementExists("#pass1")).WillOnce(Return(true));
    manager->executeAssertion(*mockBrowser, pass1);
    
    Command pass2 = createCommand("text", "#pass2", "text");
    EXPECT_CALL(*mockBrowser, getInnerText("#pass2")).WillOnce(Return("text"));
    manager->executeAssertion(*mockBrowser, pass2);
    
    Command fail1 = createCommand("exists", "#fail1");
    EXPECT_CALL(*mockBrowser, elementExists("#fail1")).WillOnce(Return(false));
    manager->executeAssertion(*mockBrowser, fail1);
    
    Command error1 = createCommand("text", "#error1", "text");
    EXPECT_CALL(*mockBrowser, getInnerText("#error1"))
        .WillOnce(testing::Throw(std::runtime_error("Error")));
    manager->executeAssertion(*mockBrowser, error1);
    
    EXPECT_EQ(manager->getTotalTests(), 4);
    EXPECT_EQ(manager->getPassedTests(), 2);
    EXPECT_EQ(manager->getFailedTests(), 1);
    EXPECT_EQ(manager->getErrorTests(), 1);
}

// ========== Output Control ==========

TEST_F(AssertionManagerTest, SilentModeControl) {
    manager->setSilentMode(true);
    // Silent mode is tested by ensuring no output is produced
    // This is more of an integration test concern
    
    Command cmd = createCommand("exists", "#test");
    EXPECT_CALL(*mockBrowser, elementExists("#test")).WillOnce(Return(true));
    
    EXPECT_NO_THROW(manager->executeAssertion(*mockBrowser, cmd));
    
    manager->setSilentMode(false);
}

TEST_F(AssertionManagerTest, JsonOutputControl) {
    manager->setJsonOutput(true);
    
    Command cmd = createCommand("exists", "#test");
    cmd.json_output = true;
    EXPECT_CALL(*mockBrowser, elementExists("#test")).WillOnce(Return(true));
    
    EXPECT_NO_THROW(manager->executeAssertion(*mockBrowser, cmd));
    
    manager->setJsonOutput(false);
}

// ========== Comparison Operations ==========

TEST_F(AssertionManagerTest, NumericComparisons) {
    // Greater than
    Command gtCmd = createCommand("count", ".item", "5", ComparisonOperator::GREATER_THAN);
    EXPECT_CALL(*mockBrowser, countElements(".item")).WillOnce(Return(10));
    EXPECT_EQ(manager->executeAssertion(*mockBrowser, gtCmd), Result::PASS);
    
    // Less than
    Command ltCmd = createCommand("count", ".item", "20", ComparisonOperator::LESS_THAN);
    EXPECT_CALL(*mockBrowser, countElements(".item")).WillOnce(Return(15));
    EXPECT_EQ(manager->executeAssertion(*mockBrowser, ltCmd), Result::PASS);
    
    // Greater than or equal
    Command geCmd = createCommand("count", ".item", "10", ComparisonOperator::GREATER_EQUAL);
    EXPECT_CALL(*mockBrowser, countElements(".item")).WillOnce(Return(10));
    EXPECT_EQ(manager->executeAssertion(*mockBrowser, geCmd), Result::PASS);
    
    // Less than or equal
    Command leCmd = createCommand("count", ".item", "15", ComparisonOperator::LESS_EQUAL);
    EXPECT_CALL(*mockBrowser, countElements(".item")).WillOnce(Return(12));
    EXPECT_EQ(manager->executeAssertion(*mockBrowser, leCmd), Result::PASS);
}

TEST_F(AssertionManagerTest, StringComparisonOperators) {
    // Not equals
    Command neCmd = createCommand("text", "#content", "wrong", ComparisonOperator::NOT_EQUALS);
    EXPECT_CALL(*mockBrowser, getInnerText("#content")).WillOnce(Return("correct"));
    EXPECT_EQ(manager->executeAssertion(*mockBrowser, neCmd), Result::PASS);
}

// ========== Edge Cases and Error Handling ==========

TEST_F(AssertionManagerTest, EmptySelectors) {
    Command cmd = createCommand("exists", "");
    
    EXPECT_CALL(*mockBrowser, elementExists(""))
        .WillOnce(testing::Throw(std::runtime_error("Empty selector")));
    
    Result result = manager->executeAssertion(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::ERROR);
}

TEST_F(AssertionManagerTest, VeryLongText) {
    std::string longText(10000, 'x');
    Command cmd = createCommand("text", "#content", longText);
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content")).WillOnce(Return(longText));
    
    Result result = manager->executeAssertion(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, UnicodeText) {
    std::string unicodeText = "测试文本 🌟 Текст";
    Command cmd = createCommand("text", "#unicode", unicodeText);
    
    EXPECT_CALL(*mockBrowser, getInnerText("#unicode")).WillOnce(Return(unicodeText));
    
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::PASS);
}

TEST_F(AssertionManagerTest, InvalidRegexPattern) {
    Command cmd = createCommand("text", "#content", "[invalid", ComparisonOperator::REGEX_MATCH);
    
    EXPECT_CALL(*mockBrowser, getInnerText("#content")).WillOnce(Return("any text"));
    
    // Invalid regex should result in error
    Result result = manager->assertText(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::ERROR);
}

TEST_F(AssertionManagerTest, CustomMessages) {
    Command cmd = createCommand("exists", "#test");
    cmd.custom_message = "Custom test message";
    
    EXPECT_CALL(*mockBrowser, elementExists("#test")).WillOnce(Return(true));
    
    manager->executeAssertion(*mockBrowser, cmd);
    
    const auto& results = manager->getResults();
    EXPECT_EQ(results.size(), 1);
    // Custom message handling would be tested in integration tests
}

TEST_F(AssertionManagerTest, TimeoutHandling) {
    Command cmd = createCommand("exists", "#slow-element");
    cmd.timeout_ms = 1000; // Short timeout
    
    // Simulate timeout scenario
    EXPECT_CALL(*mockBrowser, elementExists("#slow-element"))
        .WillOnce(testing::Throw(std::runtime_error("Timeout waiting for element")));
    
    Result result = manager->executeAssertion(*mockBrowser, cmd);
    EXPECT_EQ(result, Result::ERROR);
}

// ========== Integration-style Tests ==========

TEST_F(AssertionManagerTest, ComplexWorkflow) {
    manager->startSuite("Complex Workflow Test");
    
    // Test 1: Check page structure
    Command structureCmd = createCommand("count", ".nav-item", "5");
    EXPECT_CALL(*mockBrowser, countElements(".nav-item")).WillOnce(Return(5));
    manager->executeAssertion(*mockBrowser, structureCmd);
    
    // Test 2: Verify title
    Command titleCmd = createCommand("text", "h1", "Welcome", ComparisonOperator::CONTAINS);
    EXPECT_CALL(*mockBrowser, getInnerText("h1")).WillOnce(Return("Welcome to our site"));
    manager->executeAssertion(*mockBrowser, titleCmd);
    
    // Test 3: Check JavaScript state
    Command jsCmd = createCommand("js", "", "window.appReady === true");
    EXPECT_CALL(*mockBrowser, executeJavascriptSync("window.appReady === true")).WillOnce(Return("true"));
    manager->executeAssertion(*mockBrowser, jsCmd);
    
    // Test 4: Verify login button exists
    Command buttonCmd = createCommand("exists", "#login-button");
    EXPECT_CALL(*mockBrowser, elementExists("#login-button")).WillOnce(Return(true));
    manager->executeAssertion(*mockBrowser, buttonCmd);
    
    EXPECT_EQ(manager->getTotalTests(), 4);
    EXPECT_EQ(manager->getPassedTests(), 4);
    EXPECT_EQ(manager->getFailedTests(), 0);
    EXPECT_EQ(manager->getErrorTests(), 0);
    
    manager->endSuite();
}