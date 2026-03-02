#include <gtest/gtest.h>
#include "../../src/Assertion/Manager.h"
#include "../../src/Browser/Browser.h"
#include "../../src/Session/Session.h"
#include "../browser_test_environment.h"

extern std::unique_ptr<Browser> g_browser;

// ============================================================
// Fixture
// ============================================================

class NewAssertionTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        browser_ = g_browser.get();
        manager_ = std::make_unique<Assertion::Manager>();
        session_ = std::make_unique<Session>("test_new_assertions");
    }

    void TearDown() override {
        session_.reset();
        manager_.reset();
    }

    // Build a minimal Command with silent output so tests don't print.
    Assertion::Command makeCmd(const std::string& type,
                               const std::string& selector,
                               const std::string& expected) {
        Assertion::Command cmd;
        cmd.type = type;
        cmd.selector = selector;
        cmd.expected_value = expected;
        cmd.op = Assertion::ComparisonOperator::EQUALS;
        cmd.json_output = false;
        cmd.silent = true;
        cmd.case_sensitive = true;
        cmd.timeout_ms = 1000;
        return cmd;
    }

    // Add a header entry to the session.
    void addHeader(const std::string& url, int status,
                   const std::map<std::string, std::string>& req,
                   const std::map<std::string, std::string>& resp) {
        HttpHeaders h;
        h.url = url;
        h.method = "GET";
        h.statusCode = status;
        h.timestamp = 1709000000;
        h.requestHeaders = req;
        h.responseHeaders = resp;
        session_->addHttpHeader(h);
    }

    Browser* browser_;
    std::unique_ptr<Assertion::Manager> manager_;
    std::unique_ptr<Session> session_;
};

// ============================================================
// --assert-url
// ============================================================

TEST_F(NewAssertionTypesTest, AssertUrl_MatchesCurrentUrl) {
    EXPECT_NO_THROW({
        std::string actual_url = browser_->getCurrentUrl();
        auto cmd = makeCmd("url", "", actual_url);
        Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

TEST_F(NewAssertionTypesTest, AssertUrl_DoesNotMatch) {
    EXPECT_NO_THROW({
        auto cmd = makeCmd("url", "", "https://this-url-should-never-match.invalid/xyz");
        Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
        EXPECT_EQ(result, Assertion::Result::FAIL);
    });
}

TEST_F(NewAssertionTypesTest, AssertUrl_NotEqualsOperator) {
    EXPECT_NO_THROW({
        auto cmd = makeCmd("url", "", "!=https://this-url-should-never-match.invalid/xyz");
        Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

TEST_F(NewAssertionTypesTest, AssertUrl_RoutedThroughBrowserOnlyOverload) {
    // executeAssertion(browser, cmd) also handles "url" type
    EXPECT_NO_THROW({
        auto cmd = makeCmd("url", "", "!=this-will-not-match");
        Assertion::Result result = manager_->executeAssertion(*browser_, cmd);
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

// ============================================================
// --assert-title
// ============================================================

TEST_F(NewAssertionTypesTest, AssertTitle_MatchesCurrentTitle) {
    EXPECT_NO_THROW({
        std::string actual_title = browser_->getPageTitle();
        auto cmd = makeCmd("title", "", actual_title);
        Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

TEST_F(NewAssertionTypesTest, AssertTitle_DoesNotMatch) {
    EXPECT_NO_THROW({
        auto cmd = makeCmd("title", "", "This Title Cannot Possibly Match XYZ123");
        Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
        EXPECT_EQ(result, Assertion::Result::FAIL);
    });
}

TEST_F(NewAssertionTypesTest, AssertTitle_NotEqualsOperator) {
    EXPECT_NO_THROW({
        auto cmd = makeCmd("title", "", "!=This Title Cannot Possibly Match XYZ123");
        Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
        EXPECT_EQ(result, Assertion::Result::PASS);
    });
}

// ============================================================
// --assert-response-header
// ============================================================

TEST_F(NewAssertionTypesTest, AssertResponseHeader_EmptySession_ReturnsError) {
    auto cmd = makeCmd("response-header", "Content-Type", "application/json");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::ERROR);
}

TEST_F(NewAssertionTypesTest, AssertResponseHeader_ExactMatch_Pass) {
    addHeader("https://api.example.com/data", 200, {}, {{"Content-Type", "application/json"}});

    auto cmd = makeCmd("response-header", "Content-Type", "application/json");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

TEST_F(NewAssertionTypesTest, AssertResponseHeader_WrongValue_Fail) {
    addHeader("https://api.example.com/data", 200, {}, {{"Content-Type", "text/html"}});

    auto cmd = makeCmd("response-header", "Content-Type", "application/json");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::FAIL);
}

TEST_F(NewAssertionTypesTest, AssertResponseHeader_MissingHeader_Fail) {
    addHeader("https://api.example.com/data", 200, {}, {{"X-Other", "value"}});

    auto cmd = makeCmd("response-header", "Content-Type", "application/json");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::FAIL);
}

TEST_F(NewAssertionTypesTest, AssertResponseHeader_UsesLastEntry) {
    // Add two entries; assertion should use the last one
    addHeader("https://api.example.com/old", 200, {}, {{"Content-Type", "text/html"}});
    addHeader("https://api.example.com/new", 200, {}, {{"Content-Type", "application/json"}});

    auto cmd = makeCmd("response-header", "Content-Type", "application/json");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

TEST_F(NewAssertionTypesTest, AssertResponseHeader_NotEqualsOperator) {
    addHeader("https://api.example.com/data", 200, {}, {{"Content-Type", "application/json"}});

    auto cmd = makeCmd("response-header", "Content-Type", "!=text/html");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

// ============================================================
// --assert-request-header
// ============================================================

TEST_F(NewAssertionTypesTest, AssertRequestHeader_EmptySession_ReturnsError) {
    auto cmd = makeCmd("request-header", "Authorization", "Bearer token");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::ERROR);
}

TEST_F(NewAssertionTypesTest, AssertRequestHeader_ExactMatch_Pass) {
    addHeader("https://api.example.com/data", 200,
              {{"Authorization", "Bearer token123"}}, {});

    auto cmd = makeCmd("request-header", "Authorization", "Bearer token123");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

TEST_F(NewAssertionTypesTest, AssertRequestHeader_WrongValue_Fail) {
    addHeader("https://api.example.com/data", 200,
              {{"Authorization", "Bearer different_token"}}, {});

    auto cmd = makeCmd("request-header", "Authorization", "Bearer token123");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::FAIL);
}

TEST_F(NewAssertionTypesTest, AssertRequestHeader_MissingHeader_Fail) {
    addHeader("https://api.example.com/data", 200, {{"X-Other", "val"}}, {});

    auto cmd = makeCmd("request-header", "Authorization", "Bearer token");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::FAIL);
}

// ============================================================
// --assert-status-code
// ============================================================

TEST_F(NewAssertionTypesTest, AssertStatusCode_EmptySession_ReturnsError) {
    auto cmd = makeCmd("status-code", "", "200");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::ERROR);
}

TEST_F(NewAssertionTypesTest, AssertStatusCode_ExactMatch_Pass) {
    addHeader("https://api.example.com/data", 200, {}, {});

    auto cmd = makeCmd("status-code", "", "200");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

TEST_F(NewAssertionTypesTest, AssertStatusCode_WrongCode_Fail) {
    addHeader("https://api.example.com/data", 404, {}, {});

    auto cmd = makeCmd("status-code", "", "200");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::FAIL);
}

TEST_F(NewAssertionTypesTest, AssertStatusCode_NotEqualsOperator) {
    addHeader("https://api.example.com/data", 200, {}, {});

    auto cmd = makeCmd("status-code", "", "!=404");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

TEST_F(NewAssertionTypesTest, AssertStatusCode_GreaterThanOperator) {
    addHeader("https://api.example.com/data", 200, {}, {});

    auto cmd = makeCmd("status-code", "", ">=200");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

TEST_F(NewAssertionTypesTest, AssertStatusCode_UsesLastEntry) {
    addHeader("https://api.example.com/first", 404, {}, {});
    addHeader("https://api.example.com/second", 200, {}, {});

    auto cmd = makeCmd("status-code", "", "200");
    Assertion::Result result = manager_->executeAssertion(*browser_, *session_, cmd);
    EXPECT_EQ(result, Assertion::Result::PASS);
}

// ============================================================
// ResultTracking — new types add to results list
// ============================================================

TEST_F(NewAssertionTypesTest, NewAssertionTypes_RecordedInResults) {
    addHeader("https://api.example.com/data", 200,
              {{"Authorization", "Bearer tok"}},
              {{"Content-Type", "application/json"}});

    manager_->executeAssertion(*browser_, *session_, makeCmd("response-header", "Content-Type", "application/json"));
    manager_->executeAssertion(*browser_, *session_, makeCmd("status-code", "", "200"));

    EXPECT_EQ(manager_->getTotalTests(), 2);
    EXPECT_EQ(manager_->getPassedTests(), 2);
}
