#include <gtest/gtest.h>
#include "../../src/hweb/Config.h"
#include "../../src/hweb/Types.h"
#include "../../src/hweb/Services/ManagerRegistry.h"

using namespace HWeb;

class AssertionParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        ManagerRegistry::initialize();
    }
    
    void TearDown() override {
        ManagerRegistry::cleanup();
    }
    
    ConfigParser parser;
};

// Test --assert-exists parsing
TEST_F(AssertionParsingTest, ParseAssertExists_BasicUsage) {
    std::vector<std::string> args = {"--assert-exists", "#element"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "exists");
    EXPECT_EQ(config.assertions[0].selector, "#element");
    EXPECT_EQ(config.assertions[0].expected_value, "true");
    EXPECT_EQ(config.assertions[0].op, Assertion::ComparisonOperator::EQUALS);
}

TEST_F(AssertionParsingTest, ParseAssertExists_WithFalseExpectation) {
    std::vector<std::string> args = {"--assert-exists", "#element", "false"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "exists");
    EXPECT_EQ(config.assertions[0].selector, "#element");
    EXPECT_EQ(config.assertions[0].expected_value, "false");
}

// Test --assert-text parsing
TEST_F(AssertionParsingTest, ParseAssertText_BasicUsage) {
    std::vector<std::string> args = {"--assert-text", "h1", "Expected Title"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "text");
    EXPECT_EQ(config.assertions[0].selector, "h1");
    EXPECT_EQ(config.assertions[0].expected_value, "Expected Title");
    EXPECT_EQ(config.assertions[0].op, Assertion::ComparisonOperator::EQUALS);
}

// Test --assert-count parsing
TEST_F(AssertionParsingTest, ParseAssertCount_BasicUsage) {
    std::vector<std::string> args = {"--assert-count", ".items", "5"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "count");
    EXPECT_EQ(config.assertions[0].selector, ".items");
    EXPECT_EQ(config.assertions[0].expected_value, "5");
    EXPECT_EQ(config.assertions[0].op, Assertion::ComparisonOperator::EQUALS);
}

// Test --assert-js parsing
TEST_F(AssertionParsingTest, ParseAssertJS_BasicExpression) {
    std::vector<std::string> args = {"--assert-js", "window.testValue === 42"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "js");
    EXPECT_EQ(config.assertions[0].selector, "window.testValue === 42");
    EXPECT_EQ(config.assertions[0].expected_value, "true");
}

TEST_F(AssertionParsingTest, ParseAssertJS_WithExpectedValue) {
    std::vector<std::string> args = {"--assert-js", "window.myValue", "42"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "js");
    EXPECT_EQ(config.assertions[0].selector, "window.myValue");
    EXPECT_EQ(config.assertions[0].expected_value, "42");
}

// Test assertion modifiers
TEST_F(AssertionParsingTest, ParseAssertionWithMessage) {
    std::vector<std::string> args = {"--assert-exists", "#element", "--message", "Element should exist"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].custom_message, "Element should exist");
}

TEST_F(AssertionParsingTest, ParseAssertionWithTimeout) {
    std::vector<std::string> args = {"--assert-exists", "#element", "--timeout", "10000"};
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].timeout_ms, 10000);
}

TEST_F(AssertionParsingTest, ParseAssertionWithMessageAndTimeout) {
    std::vector<std::string> args = {
        "--assert-text", "h1", "Title", 
        "--message", "Page title verification", 
        "--timeout", "15000"
    };
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "text");
    EXPECT_EQ(config.assertions[0].selector, "h1");
    EXPECT_EQ(config.assertions[0].expected_value, "Title");
    EXPECT_EQ(config.assertions[0].custom_message, "Page title verification");
    EXPECT_EQ(config.assertions[0].timeout_ms, 15000);
}

// Test multiple assertions
TEST_F(AssertionParsingTest, ParseMultipleAssertions) {
    std::vector<std::string> args = {
        "--assert-exists", "#header",
        "--assert-text", "h1", "Welcome",
        "--assert-count", ".items", "3"
    };
    HWebConfig config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 3);
    
    EXPECT_EQ(config.assertions[0].type, "exists");
    EXPECT_EQ(config.assertions[0].selector, "#header");
    
    EXPECT_EQ(config.assertions[1].type, "text");
    EXPECT_EQ(config.assertions[1].selector, "h1");
    EXPECT_EQ(config.assertions[1].expected_value, "Welcome");
    
    EXPECT_EQ(config.assertions[2].type, "count");
    EXPECT_EQ(config.assertions[2].selector, ".items");
    EXPECT_EQ(config.assertions[2].expected_value, "3");
}

// Test test suite parsing
TEST_F(AssertionParsingTest, ParseTestSuiteStart) {
    std::vector<std::string> args = {"--test-suite", "start", "My Test Suite"};
    
    // Test suite commands are handled immediately during parsing
    // This test verifies the parsing doesn't crash
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
    });
}

TEST_F(AssertionParsingTest, ParseTestSuiteEnd) {
    std::vector<std::string> args = {"--test-suite", "end"};
    
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
    });
}

TEST_F(AssertionParsingTest, ParseTestSuiteEndWithFormat) {
    std::vector<std::string> args = {"--test-suite", "end", "json"};
    
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
    });
}

// Test mixed commands and assertions
TEST_F(AssertionParsingTest, ParseMixedCommandsAndAssertions) {
    std::vector<std::string> args = {
        "--session", "test",
        "--url", "https://example.com",
        "--assert-exists", "#content",
        "--type", "#search", "query",
        "--assert-text", ".result", "Found"
    };
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.sessionName, "test");
    EXPECT_EQ(config.url, "https://example.com");
    EXPECT_EQ(config.assertions.size(), 2);
    EXPECT_EQ(config.commands.size(), 1);
    
    EXPECT_EQ(config.assertions[0].type, "exists");
    EXPECT_EQ(config.assertions[0].selector, "#content");
    
    EXPECT_EQ(config.assertions[1].type, "text");
    EXPECT_EQ(config.assertions[1].selector, ".result");
    EXPECT_EQ(config.assertions[1].expected_value, "Found");
}

// Test JSON and silent mode inheritance
TEST_F(AssertionParsingTest, ParseAssertionsInheritJSONMode) {
    std::vector<std::string> args = {
        "--json",
        "--assert-exists", "#element"
    };
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_TRUE(config.json_mode);
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_TRUE(config.assertions[0].json_output);
}

TEST_F(AssertionParsingTest, ParseAssertionsInheritSilentMode) {
    std::vector<std::string> args = {
        "--silent",
        "--assert-exists", "#element"
    };
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_TRUE(config.silent_mode);
    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_TRUE(config.assertions[0].silent);
}

// Test error cases - note: current implementation doesn't validate args length, just parses what's available
TEST_F(AssertionParsingTest, ParseAssertExists_MissingSelector_DoesNotCrash) {
    std::vector<std::string> args = {"--assert-exists"};
    
    // Current implementation doesn't throw for missing args, just handles gracefully
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
        // Should have no assertions if parsing failed
        EXPECT_TRUE(config.assertions.empty());
    });
}

TEST_F(AssertionParsingTest, ParseAssertText_MissingArguments_DoesNotCrash) {
    std::vector<std::string> args = {"--assert-text", "#element"};
    
    // Current implementation doesn't throw for missing args, just handles gracefully
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
        // Should have no assertions if parsing failed
        EXPECT_TRUE(config.assertions.empty());
    });
}

TEST_F(AssertionParsingTest, ParseMessage_WithoutAssertion_ThrowsError) {
    std::vector<std::string> args = {"--message", "Orphaned message"};
    
    EXPECT_THROW({
        parser.parseArguments(args);
    }, std::exception);
}

TEST_F(AssertionParsingTest, ParseTimeout_WithoutAssertion_ThrowsError) {
    std::vector<std::string> args = {"--timeout", "5000"};

    EXPECT_THROW({
        parser.parseArguments(args);
    }, std::exception);
}

// ========== --assert-url parsing ==========

TEST_F(AssertionParsingTest, ParseAssertUrl_BasicUsage) {
    std::vector<std::string> args = {"--assert-url", "https://example.com"};
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "url");
    EXPECT_EQ(config.assertions[0].selector, "");
    EXPECT_EQ(config.assertions[0].expected_value, "https://example.com");
    EXPECT_EQ(config.assertions[0].op, Assertion::ComparisonOperator::EQUALS);
}

TEST_F(AssertionParsingTest, ParseAssertUrl_WithOperatorPrefix) {
    std::vector<std::string> args = {"--assert-url", "contains:dashboard"};
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "url");
    EXPECT_EQ(config.assertions[0].expected_value, "contains:dashboard");
}

TEST_F(AssertionParsingTest, ParseAssertUrl_WithMessageAndTimeout) {
    std::vector<std::string> args = {
        "--assert-url", "https://example.com/dashboard",
        "--message", "Should be on dashboard",
        "--timeout", "3000"
    };
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "url");
    EXPECT_EQ(config.assertions[0].custom_message, "Should be on dashboard");
    EXPECT_EQ(config.assertions[0].timeout_ms, 3000);
}

TEST_F(AssertionParsingTest, ParseAssertUrl_MissingArg_DoesNotCrash) {
    std::vector<std::string> args = {"--assert-url"};
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
        EXPECT_TRUE(config.assertions.empty());
    });
}

// ========== --assert-title parsing ==========

TEST_F(AssertionParsingTest, ParseAssertTitle_BasicUsage) {
    std::vector<std::string> args = {"--assert-title", "My App"};
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "title");
    EXPECT_EQ(config.assertions[0].selector, "");
    EXPECT_EQ(config.assertions[0].expected_value, "My App");
    EXPECT_EQ(config.assertions[0].op, Assertion::ComparisonOperator::EQUALS);
}

TEST_F(AssertionParsingTest, ParseAssertTitle_WithMessage) {
    std::vector<std::string> args = {
        "--assert-title", "Welcome",
        "--message", "Page title check"
    };
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "title");
    EXPECT_EQ(config.assertions[0].custom_message, "Page title check");
}

TEST_F(AssertionParsingTest, ParseAssertTitle_MissingArg_DoesNotCrash) {
    std::vector<std::string> args = {"--assert-title"};
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
        EXPECT_TRUE(config.assertions.empty());
    });
}

// ========== --assert-response-header parsing ==========

TEST_F(AssertionParsingTest, ParseAssertResponseHeader_BasicUsage) {
    std::vector<std::string> args = {"--assert-response-header", "Content-Type", "application/json"};
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "response-header");
    EXPECT_EQ(config.assertions[0].selector, "Content-Type");
    EXPECT_EQ(config.assertions[0].expected_value, "application/json");
    EXPECT_EQ(config.assertions[0].op, Assertion::ComparisonOperator::EQUALS);
}

TEST_F(AssertionParsingTest, ParseAssertResponseHeader_WithMessage) {
    std::vector<std::string> args = {
        "--assert-response-header", "Cache-Control", "no-cache",
        "--message", "Caching disabled"
    };
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].selector, "Cache-Control");
    EXPECT_EQ(config.assertions[0].custom_message, "Caching disabled");
}

TEST_F(AssertionParsingTest, ParseAssertResponseHeader_MissingArgs_DoesNotCrash) {
    std::vector<std::string> args = {"--assert-response-header", "Content-Type"};
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
        EXPECT_TRUE(config.assertions.empty());
    });
}

// ========== --assert-request-header parsing ==========

TEST_F(AssertionParsingTest, ParseAssertRequestHeader_BasicUsage) {
    std::vector<std::string> args = {"--assert-request-header", "Authorization", "Bearer token123"};
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "request-header");
    EXPECT_EQ(config.assertions[0].selector, "Authorization");
    EXPECT_EQ(config.assertions[0].expected_value, "Bearer token123");
}

// ========== --assert-status-code parsing ==========

TEST_F(AssertionParsingTest, ParseAssertStatusCode_BasicUsage) {
    std::vector<std::string> args = {"--assert-status-code", "200"};
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "status-code");
    EXPECT_EQ(config.assertions[0].selector, "");
    EXPECT_EQ(config.assertions[0].expected_value, "200");
    EXPECT_EQ(config.assertions[0].op, Assertion::ComparisonOperator::EQUALS);
}

TEST_F(AssertionParsingTest, ParseAssertStatusCode_WithOperatorPrefix) {
    std::vector<std::string> args = {"--assert-status-code", ">=200"};
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 1);
    EXPECT_EQ(config.assertions[0].type, "status-code");
    EXPECT_EQ(config.assertions[0].expected_value, ">=200");
}

TEST_F(AssertionParsingTest, ParseAssertStatusCode_MissingArg_DoesNotCrash) {
    std::vector<std::string> args = {"--assert-status-code"};
    EXPECT_NO_THROW({
        HWebConfig config = parser.parseArguments(args);
        EXPECT_TRUE(config.assertions.empty());
    });
}

// ========== Mixed new assertion types ==========

TEST_F(AssertionParsingTest, ParseMixedNewAssertionTypes) {
    std::vector<std::string> args = {
        "--assert-url", "https://example.com/dashboard",
        "--assert-title", "Dashboard",
        "--assert-status-code", "200",
        "--assert-response-header", "Content-Type", "application/json"
    };
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 4);
    EXPECT_EQ(config.assertions[0].type, "url");
    EXPECT_EQ(config.assertions[1].type, "title");
    EXPECT_EQ(config.assertions[2].type, "status-code");
    EXPECT_EQ(config.assertions[3].type, "response-header");
    EXPECT_EQ(config.assertions[3].selector, "Content-Type");
}

TEST_F(AssertionParsingTest, ParseNewAndOldAssertionsMixed) {
    std::vector<std::string> args = {
        "--assert-exists", "#nav",
        "--assert-url", "https://example.com",
        "--assert-response-header", "X-API-Version", "2"
    };
    HWebConfig config = parser.parseArguments(args);

    ASSERT_EQ(config.assertions.size(), 3);
    EXPECT_EQ(config.assertions[0].type, "exists");
    EXPECT_EQ(config.assertions[1].type, "url");
    EXPECT_EQ(config.assertions[2].type, "response-header");
}