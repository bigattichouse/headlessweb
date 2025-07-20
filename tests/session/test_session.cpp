#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Session/Session.h"
#include <json/json.h>
#include <thread>
#include <chrono>

class SessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        session = std::make_unique<Session>("test_session");
    }

    void TearDown() override {
        session.reset();
    }

    std::unique_ptr<Session> session;
};

// ========== Basic Properties Tests ==========

TEST_F(SessionTest, ConstructorSetsName) {
    EXPECT_EQ(session->getName(), "test_session");
}

TEST_F(SessionTest, SetAndGetCurrentUrl) {
    session->setCurrentUrl("https://example.com");
    EXPECT_EQ(session->getCurrentUrl(), "https://example.com");
}

// ========== Navigation History Tests ==========

TEST_F(SessionTest, InitialHistoryIsEmpty) {
    EXPECT_TRUE(session->getHistory().empty());
    EXPECT_EQ(session->getHistoryIndex(), -1);
    EXPECT_FALSE(session->canGoBack());
    EXPECT_FALSE(session->canGoForward());
}

TEST_F(SessionTest, AddToHistoryUpdatesState) {
    session->addToHistory("https://example.com");
    session->addToHistory("https://test.com");
    
    const auto& history = session->getHistory();
    EXPECT_EQ(history.size(), 2);
    EXPECT_EQ(history[0], "https://example.com");
    EXPECT_EQ(history[1], "https://test.com");
    EXPECT_EQ(session->getHistoryIndex(), 1);
}

TEST_F(SessionTest, HistoryNavigationLogic) {
    session->addToHistory("https://one.com");
    session->addToHistory("https://two.com");
    session->addToHistory("https://three.com");
    
    // At index 2 (last item)
    EXPECT_EQ(session->getHistoryIndex(), 2);
    EXPECT_TRUE(session->canGoBack());
    EXPECT_FALSE(session->canGoForward());
    
    // Go back to index 1
    session->setHistoryIndex(1);
    EXPECT_TRUE(session->canGoBack());
    EXPECT_TRUE(session->canGoForward());
    
    // Go back to index 0
    session->setHistoryIndex(0);
    EXPECT_FALSE(session->canGoBack());
    EXPECT_TRUE(session->canGoForward());
}

// ========== Cookie Management Tests ==========

TEST_F(SessionTest, InitialCookiesEmpty) {
    EXPECT_TRUE(session->getCookies().empty());
}

TEST_F(SessionTest, AddSingleCookie) {
    Cookie cookie;
    cookie.name = "test_cookie";
    cookie.value = "test_value";
    cookie.domain = "example.com";
    cookie.path = "/";
    cookie.secure = false;
    cookie.httpOnly = true;
    cookie.expires = -1; // Session cookie
    
    session->addCookie(cookie);
    
    const auto& cookies = session->getCookies();
    EXPECT_EQ(cookies.size(), 1);
    EXPECT_EQ(cookies[0].name, "test_cookie");
    EXPECT_EQ(cookies[0].value, "test_value");
    EXPECT_EQ(cookies[0].domain, "example.com");
    EXPECT_TRUE(cookies[0].httpOnly);
    EXPECT_FALSE(cookies[0].secure);
}

TEST_F(SessionTest, SetMultipleCookies) {
    std::vector<Cookie> cookies;
    
    Cookie cookie1;
    cookie1.name = "cookie1";
    cookie1.value = "value1";
    cookies.push_back(cookie1);
    
    Cookie cookie2;
    cookie2.name = "cookie2";
    cookie2.value = "value2";
    cookies.push_back(cookie2);
    
    session->setCookies(cookies);
    
    const auto& sessionCookies = session->getCookies();
    EXPECT_EQ(sessionCookies.size(), 2);
}

TEST_F(SessionTest, ClearCookies) {
    Cookie cookie;
    cookie.name = "test";
    cookie.value = "value";
    session->addCookie(cookie);
    
    EXPECT_EQ(session->getCookies().size(), 1);
    
    session->clearCookies();
    EXPECT_TRUE(session->getCookies().empty());
}

// ========== Storage Tests ==========

TEST_F(SessionTest, LocalStorageOperations) {
    EXPECT_TRUE(session->getLocalStorage().empty());
    
    session->setLocalStorageItem("key1", "value1");
    session->setLocalStorageItem("key2", "value2");
    
    const auto& storage = session->getLocalStorage();
    EXPECT_EQ(storage.size(), 2);
    EXPECT_EQ(storage.at("key1"), "value1");
    EXPECT_EQ(storage.at("key2"), "value2");
}

TEST_F(SessionTest, SessionStorageOperations) {
    EXPECT_TRUE(session->getSessionStorage().empty());
    
    session->setSessionStorageItem("session_key", "session_value");
    
    const auto& storage = session->getSessionStorage();
    EXPECT_EQ(storage.size(), 1);
    EXPECT_EQ(storage.at("session_key"), "session_value");
}

TEST_F(SessionTest, SetCompleteStorageMaps) {
    std::map<std::string, std::string> localStorage;
    localStorage["local1"] = "value1";
    localStorage["local2"] = "value2";
    
    std::map<std::string, std::string> sessionStorage;
    sessionStorage["session1"] = "svalue1";
    
    session->setLocalStorage(localStorage);
    session->setSessionStorage(sessionStorage);
    
    EXPECT_EQ(session->getLocalStorage().size(), 2);
    EXPECT_EQ(session->getSessionStorage().size(), 1);
}

// ========== Form Fields Tests ==========

TEST_F(SessionTest, FormFieldOperations) {
    EXPECT_TRUE(session->getFormFields().empty());
    
    FormField field;
    field.selector = "#username";
    field.name = "username";
    field.id = "username";
    field.type = "text";
    field.value = "testuser";
    field.checked = false;
    
    session->addFormField(field);
    
    const auto& fields = session->getFormFields();
    EXPECT_EQ(fields.size(), 1);
    EXPECT_EQ(fields[0].selector, "#username");
    EXPECT_EQ(fields[0].value, "testuser");
    EXPECT_EQ(fields[0].type, "text");
    EXPECT_FALSE(fields[0].checked);
}

TEST_F(SessionTest, CheckboxFormField) {
    FormField checkbox;
    checkbox.selector = "#accept-terms";
    checkbox.type = "checkbox";
    checkbox.checked = true;
    
    session->addFormField(checkbox);
    
    const auto& fields = session->getFormFields();
    EXPECT_EQ(fields.size(), 1);
    EXPECT_TRUE(fields[0].checked);
    EXPECT_EQ(fields[0].type, "checkbox");
}

TEST_F(SessionTest, ClearFormFields) {
    FormField field;
    field.selector = "#test";
    session->addFormField(field);
    
    EXPECT_EQ(session->getFormFields().size(), 1);
    
    session->clearFormFields();
    EXPECT_TRUE(session->getFormFields().empty());
}

// ========== Active Elements Tests ==========

TEST_F(SessionTest, ActiveElementsOperations) {
    EXPECT_TRUE(session->getActiveElements().empty());
    
    session->addActiveElement("#button1");
    session->addActiveElement("#input2");
    session->addActiveElement("#button1"); // Duplicate should not add again
    
    const auto& elements = session->getActiveElements();
    EXPECT_EQ(elements.size(), 2);
    EXPECT_TRUE(elements.count("#button1"));
    EXPECT_TRUE(elements.count("#input2"));
}

TEST_F(SessionTest, SetActiveElements) {
    std::set<std::string> elements = {"#elem1", "#elem2", "#elem3"};
    session->setActiveElements(elements);
    
    const auto& sessionElements = session->getActiveElements();
    EXPECT_EQ(sessionElements.size(), 3);
    EXPECT_TRUE(sessionElements.count("#elem1"));
    EXPECT_TRUE(sessionElements.count("#elem2"));
    EXPECT_TRUE(sessionElements.count("#elem3"));
}

// ========== Scroll Position Tests ==========

TEST_F(SessionTest, ScrollPositionOperations) {
    session->setScrollPosition("window", 100, 200);
    session->setScrollPosition("#container", 50, 75);
    
    auto windowPos = session->getScrollPosition("window");
    EXPECT_EQ(windowPos.first, 100);
    EXPECT_EQ(windowPos.second, 200);
    
    auto containerPos = session->getScrollPosition("#container");
    EXPECT_EQ(containerPos.first, 50);
    EXPECT_EQ(containerPos.second, 75);
    
    const auto& allPositions = session->getAllScrollPositions();
    EXPECT_EQ(allPositions.size(), 2);
}

TEST_F(SessionTest, DefaultScrollPosition) {
    auto defaultPos = session->getScrollPosition("nonexistent");
    EXPECT_EQ(defaultPos.first, 0);
    EXPECT_EQ(defaultPos.second, 0);
}

// ========== Page State Tests ==========

TEST_F(SessionTest, PageHashOperations) {
    session->setPageHash("#section1");
    EXPECT_EQ(session->getPageHash(), "#section1");
}

TEST_F(SessionTest, DocumentReadyState) {
    session->setDocumentReadyState("complete");
    EXPECT_EQ(session->getDocumentReadyState(), "complete");
}

// ========== Ready Conditions Tests ==========

TEST_F(SessionTest, ReadyConditionsOperations) {
    EXPECT_TRUE(session->getReadyConditions().empty());
    
    PageReadyCondition condition1;
    condition1.type = PageReadyCondition::SELECTOR;
    condition1.value = ".loading-done";
    condition1.timeout = 5000;
    
    PageReadyCondition condition2;
    condition2.type = PageReadyCondition::JS_EXPRESSION;
    condition2.value = "window.dataLoaded === true";
    condition2.timeout = 10000;
    
    session->addReadyCondition(condition1);
    session->addReadyCondition(condition2);
    
    const auto& conditions = session->getReadyConditions();
    EXPECT_EQ(conditions.size(), 2);
    EXPECT_EQ(conditions[0].type, PageReadyCondition::SELECTOR);
    EXPECT_EQ(conditions[0].value, ".loading-done");
    EXPECT_EQ(conditions[1].type, PageReadyCondition::JS_EXPRESSION);
    EXPECT_EQ(conditions[1].value, "window.dataLoaded === true");
}

TEST_F(SessionTest, ClearReadyConditions) {
    PageReadyCondition condition;
    condition.type = PageReadyCondition::CUSTOM;
    condition.value = "custom condition";
    session->addReadyCondition(condition);
    
    EXPECT_EQ(session->getReadyConditions().size(), 1);
    
    session->clearReadyConditions();
    EXPECT_TRUE(session->getReadyConditions().empty());
}

// ========== Viewport Tests ==========

TEST_F(SessionTest, ViewportOperations) {
    session->setViewport(1920, 1080);
    
    auto viewport = session->getViewport();
    EXPECT_EQ(viewport.first, 1920);
    EXPECT_EQ(viewport.second, 1080);
}

// ========== User Agent Tests ==========

TEST_F(SessionTest, UserAgentOperations) {
    std::string userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36";
    session->setUserAgent(userAgent);
    EXPECT_EQ(session->getUserAgent(), userAgent);
}

// ========== Custom Variables Tests ==========

TEST_F(SessionTest, CustomVariableOperations) {
    EXPECT_FALSE(session->hasCustomVariable("testkey"));
    EXPECT_EQ(session->getCustomVariable("testkey"), "");
    
    session->setCustomVariable("testkey", "testvalue");
    
    EXPECT_TRUE(session->hasCustomVariable("testkey"));
    EXPECT_EQ(session->getCustomVariable("testkey"), "testvalue");
}

TEST_F(SessionTest, MultipleCustomVariables) {
    session->setCustomVariable("var1", "value1");
    session->setCustomVariable("var2", "value2");
    session->setCustomVariable("var3", "value3");
    
    EXPECT_TRUE(session->hasCustomVariable("var1"));
    EXPECT_TRUE(session->hasCustomVariable("var2"));
    EXPECT_TRUE(session->hasCustomVariable("var3"));
    EXPECT_FALSE(session->hasCustomVariable("var4"));
    
    EXPECT_EQ(session->getCustomVariable("var1"), "value1");
    EXPECT_EQ(session->getCustomVariable("var2"), "value2");
    EXPECT_EQ(session->getCustomVariable("var3"), "value3");
}

// ========== State Extractors Tests ==========

TEST_F(SessionTest, StateExtractorOperations) {
    session->addStateExtractor("pageTitle", "document.title");
    session->addStateExtractor("userCount", "document.querySelectorAll('.user').length");
    
    const auto& extractors = session->getStateExtractors();
    EXPECT_EQ(extractors.size(), 2);
    EXPECT_EQ(extractors.at("pageTitle"), "document.title");
    EXPECT_EQ(extractors.at("userCount"), "document.querySelectorAll('.user').length");
}

// ========== Extracted State Tests ==========

TEST_F(SessionTest, ExtractedStateOperations) {
    Json::Value stringValue("Test Page Title");
    Json::Value numberValue(42);
    Json::Value arrayValue(Json::arrayValue);
    arrayValue.append("item1");
    arrayValue.append("item2");
    
    session->setExtractedState("title", stringValue);
    session->setExtractedState("count", numberValue);
    session->setExtractedState("items", arrayValue);
    
    EXPECT_EQ(session->getExtractedState("title").asString(), "Test Page Title");
    EXPECT_EQ(session->getExtractedState("count").asInt(), 42);
    EXPECT_EQ(session->getExtractedState("items").size(), 2);
    
    const auto& allState = session->getAllExtractedState();
    EXPECT_EQ(allState.size(), 3);
}

// ========== Action Recording Tests ==========

TEST_F(SessionTest, ActionRecordingOperations) {
    EXPECT_FALSE(session->isRecording());
    EXPECT_TRUE(session->getRecordedActions().empty());
    
    session->setRecording(true);
    EXPECT_TRUE(session->isRecording());
    
    Session::RecordedAction action1;
    action1.type = "click";
    action1.selector = "#button1";
    action1.value = "";
    action1.delay = 100;
    
    Session::RecordedAction action2;
    action2.type = "type";
    action2.selector = "#input1";
    action2.value = "test input";
    action2.delay = 50;
    
    session->recordAction(action1);
    session->recordAction(action2);
    
    const auto& actions = session->getRecordedActions();
    EXPECT_EQ(actions.size(), 2);
    EXPECT_EQ(actions[0].type, "click");
    EXPECT_EQ(actions[0].selector, "#button1");
    EXPECT_EQ(actions[1].type, "type");
    EXPECT_EQ(actions[1].value, "test input");
}

TEST_F(SessionTest, ClearRecordedActions) {
    // Enable recording first
    session->setRecording(true);
    
    Session::RecordedAction action;
    action.type = "click";
    action.selector = "#test";
    session->recordAction(action);
    
    EXPECT_EQ(session->getRecordedActions().size(), 1);
    
    session->clearRecordedActions();
    EXPECT_EQ(session->getRecordedActions().size(), 0);
}

// ========== Session Metadata Tests ==========

TEST_F(SessionTest, LastAccessedOperations) {
    int64_t originalTime = session->getLastAccessed();
    
    // Wait to ensure timestamp changes (since we use second precision)
    std::this_thread::sleep_for(std::chrono::milliseconds(1001));
    
    // Update should change the timestamp
    session->updateLastAccessed();
    int64_t updatedTime = session->getLastAccessed();
    
    EXPECT_GT(updatedTime, originalTime);
}

TEST_F(SessionTest, ApproximateSize) {
    // Empty session should have minimal size
    size_t emptySize = session->getApproximateSize();
    EXPECT_GT(emptySize, 0);
    
    // Add some data and size should increase
    session->setCurrentUrl("https://example.com");
    session->addToHistory("https://test.com");
    session->setCustomVariable("key", "value");
    
    size_t filledSize = session->getApproximateSize();
    EXPECT_GT(filledSize, emptySize);
}

// ========== Serialization Tests ==========

TEST_F(SessionTest, SerializationRoundTrip) {
    // Set up session with various data
    session->setCurrentUrl("https://example.com");
    session->addToHistory("https://first.com");
    session->addToHistory("https://second.com");
    
    Cookie cookie;
    cookie.name = "test";
    cookie.value = "value";
    cookie.domain = "example.com";
    session->addCookie(cookie);
    
    session->setLocalStorageItem("key", "value");
    session->setCustomVariable("var", "val");
    session->setViewport(1920, 1080);
    
    // Serialize
    std::string serialized = session->serialize();
    EXPECT_FALSE(serialized.empty());
    
    // Deserialize
    Session restored = Session::deserialize(serialized);
    
    // Verify data integrity
    EXPECT_EQ(restored.getName(), session->getName());
    EXPECT_EQ(restored.getCurrentUrl(), session->getCurrentUrl());
    EXPECT_EQ(restored.getHistory().size(), session->getHistory().size());
    EXPECT_EQ(restored.getCookies().size(), session->getCookies().size());
    EXPECT_EQ(restored.getLocalStorage().size(), session->getLocalStorage().size());
    EXPECT_EQ(restored.getCustomVariable("var"), "val");
    
    auto viewport = restored.getViewport();
    EXPECT_EQ(viewport.first, 1920);
    EXPECT_EQ(viewport.second, 1080);
}

TEST_F(SessionTest, SerializationWithComplexData) {
    // Add complex form fields
    FormField field1;
    field1.selector = "#username";
    field1.name = "username";
    field1.type = "text";
    field1.value = "testuser";
    field1.checked = false;
    
    FormField field2;
    field2.selector = "#remember";
    field2.name = "remember";
    field2.type = "checkbox";
    field2.checked = true;
    
    session->addFormField(field1);
    session->addFormField(field2);
    
    // Add ready conditions
    PageReadyCondition condition;
    condition.type = PageReadyCondition::SELECTOR;
    condition.value = ".loading-complete";
    condition.timeout = 5000;
    session->addReadyCondition(condition);
    
    // Add recorded actions
    Session::RecordedAction action;
    action.type = "click";
    action.selector = "#submit";
    action.delay = 200;
    session->setRecording(true);
    session->recordAction(action);
    
    // Test serialization round trip
    std::string serialized = session->serialize();
    Session restored = Session::deserialize(serialized);
    
    // Verify complex data
    const auto& restoredFields = restored.getFormFields();
    EXPECT_EQ(restoredFields.size(), 2);
    EXPECT_EQ(restoredFields[0].selector, "#username");
    EXPECT_EQ(restoredFields[1].type, "checkbox");
    EXPECT_TRUE(restoredFields[1].checked);
    
    const auto& restoredConditions = restored.getReadyConditions();
    EXPECT_EQ(restoredConditions.size(), 1);
    EXPECT_EQ(restoredConditions[0].value, ".loading-complete");
    
    const auto& restoredActions = restored.getRecordedActions();
    EXPECT_EQ(restoredActions.size(), 1);
    EXPECT_EQ(restoredActions[0].type, "click");
    EXPECT_EQ(restoredActions[0].selector, "#submit");
}

// ========== Edge Cases and Error Handling ==========

TEST_F(SessionTest, EmptyStringHandling) {
    session->setCurrentUrl("");
    EXPECT_EQ(session->getCurrentUrl(), "");
    
    session->setCustomVariable("", "value");
    session->setCustomVariable("key", "");
    EXPECT_EQ(session->getCustomVariable(""), "value");
    EXPECT_EQ(session->getCustomVariable("key"), "");
}

TEST_F(SessionTest, LargeDataHandling) {
    // Test with large strings
    std::string largeUrl(10000, 'x');
    session->setCurrentUrl(largeUrl);
    EXPECT_EQ(session->getCurrentUrl().size(), 10000);
    
    // Test with many history items (respects MAX_HISTORY limit of 100)
    for (int i = 0; i < 150; i++) {  // Add more than limit to test truncation
        session->addToHistory("https://test" + std::to_string(i) + ".com");
    }
    // Should be capped at MAX_HISTORY limit (100)
    EXPECT_EQ(session->getHistory().size(), 100);
    
    // Verify the latest items are kept (not the earliest)
    const auto& history = session->getHistory();
    EXPECT_EQ(history.back(), "https://test149.com");  // Last item added
}

TEST_F(SessionTest, UnicodeHandling) {
    std::string unicodeUrl = "https://example.com/测试页面";
    session->setCurrentUrl(unicodeUrl);
    EXPECT_EQ(session->getCurrentUrl(), unicodeUrl);
    
    session->setCustomVariable("测试键", "测试值");
    EXPECT_EQ(session->getCustomVariable("测试键"), "测试值");
}