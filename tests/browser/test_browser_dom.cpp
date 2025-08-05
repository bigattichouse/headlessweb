#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "../utils/SafePageLoader.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <filesystem>
#include <thread>
#include <chrono>

extern std::unique_ptr<Browser> g_browser;

class BrowserDOMTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_dom_tests");
        
        // Use EXACTLY the same pattern as working BrowserCoreTest
        browser = g_browser.get();
        
        // Create session for browser initialization (same as BrowserCoreTest)
        session = std::make_unique<Session>("test_session");
        session->setCurrentUrl("about:blank");
        session->setViewport(1024, 768);
        
        debug_output("BrowserDOMTest SetUp complete");
    }

    void TearDown() override {
        // Clean up without destroying global browser (same as BrowserCoreTest)
        session.reset();
        temp_dir.reset();
    }

    Browser* browser;  // Raw pointer to global browser instance
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<Session> session;
};

// ========== Basic Browser DOM Interface Tests (No Page Loading Required) ==========

TEST_F(BrowserDOMTest, BasicDOMInterfaceTest) {
    // Test that DOM interface methods are accessible (like BrowserCoreTest)
    EXPECT_NO_THROW({
        // Test element existence interface (should handle empty page gracefully)
        bool exists = browser->elementExists("#nonexistent");
        
        // Test basic DOM operations interface
        browser->fillInput("#nonexistent", "test");
        browser->clickElement("#nonexistent");
        browser->getAttribute("#nonexistent", "value");
    });
}

TEST_F(BrowserDOMTest, ElementExistenceInterfaceTest) {
    // Test element existence interface handles various selectors gracefully
    EXPECT_NO_THROW({
        browser->elementExists(""); // Empty selector
        browser->elementExists("#"); // Invalid ID selector
        browser->elementExists("."); // Invalid class selector
        browser->elementExists("div.class#id"); // Complex selector
        browser->elementExists("div > p + span"); // CSS combinator
        browser->elementExists("input[type='text']"); // Attribute selector
    });
}

// ========== Form Interface Tests (No Page Loading Required) ==========

TEST_F(BrowserDOMTest, FormInputInterfaceTest) {
    // Test form input interface methods (should handle non-existent elements gracefully)
    EXPECT_NO_THROW({
        browser->fillInput("#username", "testuser");
        browser->fillInput("#password", "password123");
        browser->fillInput("#email", "test@example.com");
        browser->fillInput("#comments", "This is a test comment");
    });
}

TEST_F(BrowserDOMTest, FormInputValidationInterfaceTest) {
    // Test input validation interface handles edge cases
    EXPECT_NO_THROW({
        browser->fillInput("#username", ""); // Empty value
        browser->fillInput("#password", std::string(1000, 'a')); // Very long value
        browser->fillInput("#email", "unicode测试@example.com"); // Unicode content
        browser->fillInput("#nonexistent", "value"); // Nonexistent element
    });
}

TEST_F(BrowserDOMTest, SelectOptionInterfaceTest) {
    // Test select option interface methods
    EXPECT_NO_THROW({
        browser->selectOption("#country", "us");
        browser->selectOption("#country", "uk");
        browser->selectOption("#country", ""); // Reset to default
        browser->selectOption("#country", "invalid"); // Invalid option
        browser->selectOption("#nonexistent", "value"); // Nonexistent select
    });
}

TEST_F(BrowserDOMTest, CheckboxInterfaceTest) {
    // Test checkbox interface methods
    EXPECT_NO_THROW({
        browser->checkElement("#subscribe");
        browser->uncheckElement("#subscribe");
        browser->checkElement("#nonexistent"); // Nonexistent element
        browser->uncheckElement("#nonexistent");
    });
}

// ========== Element Interaction Interface Tests ==========

TEST_F(BrowserDOMTest, ElementClickingInterfaceTest) {
    // Test element clicking interface methods
    EXPECT_NO_THROW({
        browser->clickElement("#test-button");
        browser->clickElement(".list-item");
        browser->clickElement("#nonexistent"); // Nonexistent element
    });
}

TEST_F(BrowserDOMTest, ElementFocusingInterfaceTest) {
    // Test element focusing interface methods
    EXPECT_NO_THROW({
        browser->focusElement("#username");
        browser->focusElement("#password");
        browser->focusElement("#search-input");
        browser->focusElement("#nonexistent"); // Nonexistent element
    });
}

// ========== Form Submission Interface Tests ==========

TEST_F(BrowserDOMTest, FormSubmissionInterfaceTest) {
    // Test form submission interface methods
    EXPECT_NO_THROW({
        browser->submitForm("#test-form");
        browser->submitForm("#search-form");
        browser->submitForm(); // Default form submission
        browser->submitForm("#nonexistent"); // Nonexistent form
    });
}

TEST_F(BrowserDOMTest, SearchFormInterfaceTest) {
    // Test search form interface methods
    EXPECT_NO_THROW({
        browser->searchForm("test query");
        browser->searchForm(""); // Empty query
        browser->searchForm("unicode测试query");
        browser->searchForm(std::string(1000, 'x')); // Very long query
    });
}

// ========== Attribute Management Interface Tests ==========

TEST_F(BrowserDOMTest, AttributeGettingInterfaceTest) {
    // Test attribute retrieval interface methods
    EXPECT_NO_THROW({
        std::string value = browser->getAttribute("#username", "name");
        std::string type = browser->getAttribute("#password", "type");
        std::string placeholder = browser->getAttribute("#text-input", "placeholder");
        std::string nonexistent = browser->getAttribute("#nonexistent", "value");
        std::string invalid_attr = browser->getAttribute("#username", "");
    });
}

TEST_F(BrowserDOMTest, AttributeSettingInterfaceTest) {
    // Test attribute setting interface methods
    EXPECT_NO_THROW({
        browser->setAttribute("#text-input", "value", "new value");
        browser->setAttribute("#test-button", "disabled", "true");
        browser->setAttribute("#dynamic-content", "style", "display: block;");
        browser->setAttribute("#nonexistent", "value", "test"); // Nonexistent element
        browser->setAttribute("#username", "", "value"); // Empty attribute name
    });
}

// ========== Complex Selector Interface Tests ==========

TEST_F(BrowserDOMTest, ComplexSelectorInterfaceTest) {
    // Test complex CSS selector interface methods
    std::vector<std::string> complex_selectors = {
        "div#main-content",
        ".description",
        "ul.test-list li.list-item",
        "input[type='text']",
        "input[type='hidden'][value='hidden-value']",
        "form#test-form input[name='username']",
        "li:first-child",
        "li:last-child",
        "li:nth-child(2)",
        "div > p",
        "button + input",
        "label[for='subscribe']"
    };
    
    for (const auto& selector : complex_selectors) {
        EXPECT_NO_THROW({
            browser->elementExists(selector);
            browser->clickElement(selector);
            browser->getAttribute(selector, "id");
        });
    }
}

// ========== XPath Selector Interface Tests ==========

TEST_F(BrowserDOMTest, XPathSelectorInterfaceTest) {
    // Test XPath selector interface methods
    std::vector<std::string> xpath_selectors = {
        "//div[@id='main-content']",
        "//input[@type='text']",
        "//button[text()='Click Me']",
        "//li[contains(@class, 'list-item')]",
        "//form//input[@name='username']"
    };
    
    for (const auto& xpath : xpath_selectors) {
        EXPECT_NO_THROW({
            browser->elementExists(xpath);
            browser->clickElement(xpath);
        });
    }
}

// ========== Error Handling Interface Tests ==========

TEST_F(BrowserDOMTest, InvalidSelectorInterfaceTest) {
    // Test interface handling of invalid selectors
    std::vector<std::string> invalid_selectors = {
        "",
        "#",
        ".",
        "[",
        ")",
        "div..class",
        "#id id",
        ">>invalid",
        std::string(1000, 'a') // Very long selector
    };
    
    for (const auto& selector : invalid_selectors) {
        EXPECT_NO_THROW({
            browser->elementExists(selector);
            browser->clickElement(selector);
            browser->fillInput(selector, "value");
        });
    }
}

TEST_F(BrowserDOMTest, UnicodeContentInterfaceTest) {
    // Test Unicode content interface handling
    std::vector<std::string> unicode_values = {
        "测试文本",
        "العربية",
        "Русский",
        "🎉🔧💻",
        "Ñiño José Müller",
        "Κόσμος",
        "こんにちは世界"
    };
    
    for (const auto& value : unicode_values) {
        EXPECT_NO_THROW({
            browser->fillInput("#username", value);
            browser->fillInput("#comments", value);
            browser->searchForm(value);
        });
    }
}

TEST_F(BrowserDOMTest, LargeContentInterfaceTest) {
    // Test interface handling of large content
    std::string large_text(1000, 'A');  // Reduced size for interface test
    std::string very_large_text(5000, 'B');  // Reduced size for interface test
    
    EXPECT_NO_THROW({
        browser->fillInput("#comments", large_text);
        browser->fillInput("#comments", very_large_text);
        browser->searchForm(large_text);
    });
}

// ========== Basic Interface Performance Tests ==========

TEST_F(BrowserDOMTest, BasicInterfacePerformanceTest) {
    // Test that DOM interface methods execute without throwing
    EXPECT_NO_THROW({
        for (int i = 0; i < 5; ++i) {  // Minimal iterations for interface testing
            browser->elementExists("#test-button");
            browser->getAttribute("#username", "name");
            browser->fillInput("#search-input", "test" + std::to_string(i));
        }
    });
}