#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <memory>
#include <thread>
#include <chrono>

extern bool g_debug;
extern std::unique_ptr<Browser> g_browser;

class BrowserFormOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug output for tests
        g_debug = true;
        
        // Use global browser instance like other working tests
        browser = g_browser.get();
    }
    
    void TearDown() override {
        // Clean teardown without navigation
    }

    Browser* browser;
};

// ========== Form Interface Tests (Following BrowserDOMTest Pattern) ==========

TEST_F(BrowserFormOperationsTest, BasicFormInterfaceTest) {
    // Test that form interface methods are accessible (like BrowserCoreTest)
    EXPECT_NO_THROW({
        // Test basic form operations interface (should handle empty page gracefully)
        browser->fillInput("#nonexistent-input", "test");
        browser->clickElement("#nonexistent-button");
        browser->submitForm("#nonexistent-form");
    });
}

TEST_F(BrowserFormOperationsTest, FormElementExistenceInterfaceTest) {
    // Test form element existence checking interface
    EXPECT_NO_THROW({
        // Test element existence interface for form elements
        bool input_exists = browser->elementExists("#nonexistent-input");
        bool button_exists = browser->elementExists("button");
        bool form_exists = browser->elementExists("form");
        
        // Test counting form elements
        int input_count = browser->countElements("input");
        int form_count = browser->countElements("form");
    });
}

TEST_F(BrowserFormOperationsTest, FormInputInterfaceTest) {
    // Test form input interface methods
    EXPECT_NO_THROW({
        // Test input interface methods (should handle gracefully on empty page)
        browser->fillInput("#text-input", "test value");
        browser->fillInput("#email-input", "test@example.com");
        browser->fillInput("#password-input", "password123");
        
        // Test input validation interface
        std::string value = browser->getAttribute("#text-input", "value");
        std::string type = browser->getAttribute("#email-input", "type");
    });
}

TEST_F(BrowserFormOperationsTest, FormButtonInterfaceTest) {
    // Test form button interface methods
    EXPECT_NO_THROW({
        // Test button clicking interface
        browser->clickElement("#submit-button");
        browser->clickElement("button[type='submit']");
        browser->clickElement("input[type='submit']");
        
        // Test button attribute interface
        std::string button_type = browser->getAttribute("button", "type");
        std::string button_value = browser->getAttribute("input[type='submit']", "value");
    });
}

TEST_F(BrowserFormOperationsTest, SelectDropdownInterfaceTest) {
    // Test select dropdown interface methods
    EXPECT_NO_THROW({
        // Test select operations interface
        browser->selectOption("#dropdown", "option1");
        browser->selectOption("select", "value2");
        browser->clickElement("select option");
        
        // Test select attribute interface
        std::string selected = browser->getAttribute("select", "value");
        bool multiple = (browser->getAttribute("select", "multiple") != "");
    });
}

TEST_F(BrowserFormOperationsTest, CheckboxRadioInterfaceTest) {
    // Test checkbox and radio button interface methods
    EXPECT_NO_THROW({
        // Test checkbox/radio interface
        browser->clickElement("input[type='checkbox']");
        browser->clickElement("input[type='radio']");
        
        // Test checkbox/radio attributes interface
        std::string checked = browser->getAttribute("input[type='checkbox']", "checked");
        std::string value = browser->getAttribute("input[type='radio']", "value");
        std::string name = browser->getAttribute("input[type='radio']", "name");
    });
}

TEST_F(BrowserFormOperationsTest, FormSubmissionInterfaceTest) {
    // Test form submission interface methods
    EXPECT_NO_THROW({
        // Test form submission interface
        browser->submitForm("#test-form");
        browser->submitForm("form");
        
        // Test form element access interface
        std::string action = browser->getAttribute("form", "action");
        std::string method = browser->getAttribute("form", "method");
        bool form_exists = browser->elementExists("form");
    });
}

TEST_F(BrowserFormOperationsTest, FormElementAttributesInterfaceTest) {
    // Test form element attributes interface
    EXPECT_NO_THROW({
        // Test various form element attributes
        browser->getAttribute("input", "name");
        browser->getAttribute("input", "id");
        browser->getAttribute("input", "class");
        browser->getAttribute("input", "placeholder");
        browser->getAttribute("input", "required");
        browser->getAttribute("input", "disabled");
        browser->getAttribute("input", "readonly");
    });
}

TEST_F(BrowserFormOperationsTest, FormValidationInterfaceTest) {
    // Test form validation interface methods
    EXPECT_NO_THROW({
        // Test validation interface methods
        std::string validation_message = browser->getAttribute("input", "validationMessage");
        std::string pattern = browser->getAttribute("input", "pattern");
        std::string min = browser->getAttribute("input", "min");
        std::string max = browser->getAttribute("input", "max");
        std::string step = browser->getAttribute("input", "step");
    });
}

TEST_F(BrowserFormOperationsTest, FormComplexSelectorsInterfaceTest) {
    // Test form operations with complex selectors
    EXPECT_NO_THROW({
        // Test complex selectors for form elements
        browser->clickElement("form input[type='submit']");
        browser->fillInput("form input[name='username']", "test");
        browser->selectOption("form select[name='country']", "US");
        browser->clickElement("form input[type='checkbox'][name='agree']");
        
        // Test descendant and adjacent selectors
        browser->clickElement("label + input");
        browser->fillInput("fieldset input", "test");
        browser->getAttribute("form > input", "value");
    });
}

TEST_F(BrowserFormOperationsTest, FormXPathSelectorsInterfaceTest) {
    // Test form operations with XPath selectors
    EXPECT_NO_THROW({
        // Test XPath selectors for form elements (should handle gracefully)
        browser->clickElement("//input[@type='submit']");
        browser->fillInput("//input[@name='email']", "test@example.com");
        browser->selectOption("//select[@id='dropdown']", "option1");
        browser->getAttribute("//form//input", "value");
        
        // Test XPath with predicates
        browser->clickElement("//input[@type='checkbox'][1]");
        browser->fillInput("//form[1]//input[@type='text']", "test");
    });
}

TEST_F(BrowserFormOperationsTest, FormInvalidSelectorsInterfaceTest) {
    // Test form operations with invalid selectors
    EXPECT_NO_THROW({
        // Test interface handles invalid selectors gracefully
        browser->fillInput("", "test");
        browser->clickElement("invalid-selector");
        browser->selectOption("[unclosed", "option");
        browser->getAttribute("malformed[", "value");
        
        // Test empty/null values
        browser->fillInput("#input", "");
        browser->selectOption("#select", "");
        browser->getAttribute("#element", "");
    });
}

TEST_F(BrowserFormOperationsTest, FormUnicodeContentInterfaceTest) {
    // Test form operations with unicode content
    EXPECT_NO_THROW({
        // Test unicode content in form operations
        browser->fillInput("#text-input", "Unicode: 你好世界 🌍 αβγ");
        browser->fillInput("#email-input", "test@例え.テスト");
        browser->selectOption("#select", "选项");
        
        // Test unicode selectors and attributes
        browser->getAttribute("#元素", "值");
        browser->clickElement("[title='测试']");
    });
}

TEST_F(BrowserFormOperationsTest, FormLargeContentInterfaceTest) {
    // Test form operations with large content
    EXPECT_NO_THROW({
        // Test large content handling in form operations
        std::string large_text(10000, 'A');
        browser->fillInput("#textarea", large_text);
        
        std::string large_value(5000, 'B');
        browser->fillInput("#input", large_value);
        
        // Test retrieving large attribute values
        std::string large_attr = browser->getAttribute("#element", "data-large");
    });
}

TEST_F(BrowserFormOperationsTest, FormPerformanceInterfaceTest) {
    // Test form operations performance
    EXPECT_NO_THROW({
        // Test repeated form operations (interface should handle efficiently)
        for (int i = 0; i < 10; ++i) {
            browser->fillInput("#input" + std::to_string(i), "value" + std::to_string(i));
            browser->clickElement("#button" + std::to_string(i));
            browser->selectOption("#select" + std::to_string(i), "option" + std::to_string(i));
            browser->getAttribute("#element" + std::to_string(i), "data-test");
        }
    });
}

TEST_F(BrowserFormOperationsTest, FormEdgeCasesInterfaceTest) {
    // Test form operations edge cases
    EXPECT_NO_THROW({
        // Test edge case scenarios
        browser->fillInput("#input[data-test='value with spaces']", "test");
        browser->clickElement("button:contains('Click Me')");
        browser->selectOption("select:has(option[value='test'])", "test");
        
        // Test special characters in selectors
        browser->getAttribute("#element\\:with\\:colons", "value");
        browser->fillInput("#input\\.with\\.dots", "test");
        browser->clickElement("#button\\@with\\@symbols");
    });
}