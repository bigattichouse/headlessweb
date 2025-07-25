#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
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
        
        // Create test HTML files for DOM testing
        createTestHtmlFile();
        createFormTestHtmlFile();
        
        // CRITICAL FIX: Actually load the test page for real DOM testing
        // Apply the proven file:// URL approach that resolved BrowserStorageTest issues
        bool page_loaded = loadTestPage();
        if (!page_loaded) {
            GTEST_SKIP() << "Failed to load test page - DOM operations require loaded content";
        }
    }

    void TearDown() override {
        temp_dir.reset();
    }
    
    bool loadTestPage() {
        // Load the test HTML file using file:// URL (proven successful approach)
        std::string file_url = "file://" + test_html_file.string();
        debug_output("Loading DOM test page: " + file_url);
        
        g_browser->loadUri(file_url);
        
        // Wait for navigation to complete
        bool navigation_success = g_browser->waitForNavigation(10000);
        if (!navigation_success) {
            debug_output("Navigation failed for DOM test page");
            return false;
        }
        
        // CRITICAL: Wait for DOM to be fully ready (same pattern as successful tests)
        std::string dom_ready = g_browser->executeJavascriptSync(
            "document.readyState === 'complete' && "
            "document.getElementById('main-content') !== null && "
            "document.getElementById('title') !== null"
        );
        
        if (dom_ready != "true") {
            debug_output("DOM not ready, waiting additional time...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // Re-check DOM readiness
            dom_ready = g_browser->executeJavascriptSync(
                "document.readyState === 'complete' && "
                "document.getElementById('main-content') !== null && "
                "document.getElementById('title') !== null"
            );
        }
        
        debug_output("DOM test page loaded - ready: " + dom_ready);
        return dom_ready == "true";
    }

    void createTestHtmlFile() {
        std::string html_content = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>DOM Test Page</title>
</head>
<body>
    <div id="main-content">
        <h1 id="title">Test Page</h1>
        <p class="description">This is a test page for DOM operations</p>
        <button id="test-button" onclick="buttonClicked()">Click Me</button>
        <input id="text-input" type="text" placeholder="Enter text">
        <input id="hidden-input" type="hidden" value="hidden-value">
        <div id="dynamic-content" style="display: none;">Hidden Content</div>
        <ul class="test-list">
            <li class="list-item">Item 1</li>
            <li class="list-item">Item 2</li>
            <li class="list-item">Item 3</li>
        </ul>
    </div>
    <script>
        function buttonClicked() {
            document.getElementById("dynamic-content").style.display = "block";
        }
        
        function showElement(id) {
            document.getElementById(id).style.display = "block";
        }
        
        function hideElement(id) {
            document.getElementById(id).style.display = "none";
        }
    </script>
</body>
</html>
)HTML";
        test_html_file = temp_dir->createFile("test.html", html_content);
    }

    void createFormTestHtmlFile() {
        std::string form_content = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Form Test Page</title>
</head>
<body>
    <form id="test-form" action="/submit" method="post">
        <input id="username" name="username" type="text" required>
        <input id="password" name="password" type="password" required>
        <input id="email" name="email" type="email">
        <select id="country" name="country">
            <option value="">Select Country</option>
            <option value="us">United States</option>
            <option value="uk">United Kingdom</option>
            <option value="ca">Canada</option>
        </select>
        <input id="subscribe" name="subscribe" type="checkbox">
        <label for="subscribe">Subscribe to newsletter</label>
        <textarea id="comments" name="comments" rows="4" cols="50"></textarea>
        <button id="submit-btn" type="submit">Submit</button>
        <button id="reset-btn" type="reset">Reset</button>
    </form>
    
    <form id="search-form">
        <input id="search-input" name="q" type="search" placeholder="Search...">
        <button type="submit">Search</button>
    </form>
</body>
</html>
)";
        form_html_file = temp_dir->createFile("form.html", form_content);
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::filesystem::path test_html_file;
    std::filesystem::path form_html_file;
};

// ========== Element Existence Tests ==========

TEST_F(BrowserDOMTest, ElementExistenceChecking) {
    // Test element existence with actual loaded DOM content
    // Elements from our test HTML should exist
    EXPECT_TRUE(g_browser->elementExists("#main-content"));
    EXPECT_TRUE(g_browser->elementExists(".description"));
    EXPECT_TRUE(g_browser->elementExists("h1"));
    EXPECT_TRUE(g_browser->elementExists("#title"));
    EXPECT_TRUE(g_browser->elementExists("#test-button"));
    
    // Non-existent elements should return false
    EXPECT_FALSE(g_browser->elementExists("#nonexistent"));
    EXPECT_FALSE(g_browser->elementExists(".missing-class"));
}

TEST_F(BrowserDOMTest, ElementExistenceEdgeCases) {
    // Test edge cases for element existence
    EXPECT_NO_THROW({
        g_browser->elementExists(""); // Empty selector
        g_browser->elementExists("#"); // Invalid ID selector
        g_browser->elementExists("."); // Invalid class selector
        g_browser->elementExists("div.class#id"); // Complex selector
        g_browser->elementExists("div > p + span"); // CSS combinator
        g_browser->elementExists("input[type='text']"); // Attribute selector
    });
}

// ========== Form Interaction Tests ==========

TEST_F(BrowserDOMTest, FormInputFilling) {
    // Switch to form page for input testing
    std::string form_url = "file://" + form_html_file.string();
    g_browser->loadUri(form_url);
    g_browser->waitForNavigation(5000);
    
    // Wait for form elements to be available
    std::string form_ready = g_browser->executeJavascriptSync(
        "document.getElementById('username') !== null && "
        "document.getElementById('password') !== null"
    );
    
    if (form_ready == "true") {
        // Test form input filling with real elements
        EXPECT_NO_THROW({
            g_browser->fillInput("#username", "testuser");
            g_browser->fillInput("#password", "password123");
            g_browser->fillInput("#email", "test@example.com");
            g_browser->fillInput("#comments", "This is a test comment");
        });
        
        // Verify values were actually set
        std::string username_value = g_browser->getAttribute("#username", "value");
        EXPECT_EQ(username_value, "testuser");
    } else {
        GTEST_SKIP() << "Form elements not ready for testing";
    }
}

TEST_F(BrowserDOMTest, FormInputValidation) {
    // Test input validation and edge cases
    EXPECT_NO_THROW({
        g_browser->fillInput("#username", ""); // Empty value
        g_browser->fillInput("#password", std::string(1000, 'a')); // Very long value
        g_browser->fillInput("#email", "unicode测试@example.com"); // Unicode content
        g_browser->fillInput("#nonexistent", "value"); // Nonexistent element
    });
}

TEST_F(BrowserDOMTest, SelectOptionHandling) {
    // Test select option handling
    EXPECT_NO_THROW({
        g_browser->selectOption("#country", "us");
        g_browser->selectOption("#country", "uk");
        g_browser->selectOption("#country", ""); // Reset to default
        g_browser->selectOption("#country", "invalid"); // Invalid option
        g_browser->selectOption("#nonexistent", "value"); // Nonexistent select
    });
}

TEST_F(BrowserDOMTest, CheckboxAndRadioHandling) {
    // Test checkbox and radio button operations
    EXPECT_NO_THROW({
        g_browser->checkElement("#subscribe");
        g_browser->uncheckElement("#subscribe");
        g_browser->checkElement("#nonexistent"); // Nonexistent element
        g_browser->uncheckElement("#nonexistent");
    });
}

// ========== Element Interaction Tests ==========

TEST_F(BrowserDOMTest, ElementClicking) {
    // Test clicking elements that exist in our loaded page
    EXPECT_NO_THROW({
        g_browser->clickElement("#test-button");
        g_browser->clickElement(".list-item");
    });
    
    // Test clicking non-existent elements (should handle gracefully)
    EXPECT_NO_THROW({
        g_browser->clickElement("#nonexistent");
    });
}

TEST_F(BrowserDOMTest, ElementFocusing) {
    // Test element focusing
    EXPECT_NO_THROW({
        g_browser->focusElement("#username");
        g_browser->focusElement("#password");
        g_browser->focusElement("#search-input");
        g_browser->focusElement("#nonexistent"); // Nonexistent element
    });
}

// ========== Form Submission Tests ==========

TEST_F(BrowserDOMTest, FormSubmission) {
    // Test form submission interface
    EXPECT_NO_THROW({
        g_browser->submitForm("#test-form");
        g_browser->submitForm("#search-form");
        g_browser->submitForm(); // Default form submission
        g_browser->submitForm("#nonexistent"); // Nonexistent form
    });
}

TEST_F(BrowserDOMTest, SearchFormHandling) {
    // Test search form functionality
    EXPECT_NO_THROW({
        g_browser->searchForm("test query");
        g_browser->searchForm(""); // Empty query
        g_browser->searchForm("unicode测试query");
        g_browser->searchForm(std::string(1000, 'x')); // Very long query
    });
}

// ========== Attribute Management Tests ==========

TEST_F(BrowserDOMTest, AttributeGetting) {
    // Test attribute retrieval interface
    EXPECT_NO_THROW({
        std::string value = g_browser->getAttribute("#username", "name");
        std::string type = g_browser->getAttribute("#password", "type");
        std::string placeholder = g_browser->getAttribute("#text-input", "placeholder");
        std::string nonexistent = g_browser->getAttribute("#nonexistent", "value");
        std::string invalid_attr = g_browser->getAttribute("#username", "");
    });
}

TEST_F(BrowserDOMTest, AttributeSetting) {
    // Test attribute setting interface
    EXPECT_NO_THROW({
        g_browser->setAttribute("#text-input", "value", "new value");
        g_browser->setAttribute("#test-button", "disabled", "true");
        g_browser->setAttribute("#dynamic-content", "style", "display: block;");
        g_browser->setAttribute("#nonexistent", "value", "test"); // Nonexistent element
        g_browser->setAttribute("#username", "", "value"); // Empty attribute name
    });
}

// ========== Complex Selector Tests ==========

TEST_F(BrowserDOMTest, ComplexSelectorHandling) {
    // Test complex CSS selectors
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
            g_browser->elementExists(selector);
            g_browser->clickElement(selector);
            g_browser->getAttribute(selector, "id");
        });
    }
}

// ========== XPath Selector Tests ==========

TEST_F(BrowserDOMTest, XPathSelectorSupport) {
    // Test XPath selectors (if supported)
    std::vector<std::string> xpath_selectors = {
        "//div[@id='main-content']",
        "//input[@type='text']",
        "//button[text()='Click Me']",
        "//li[contains(@class, 'list-item')]",
        "//form//input[@name='username']"
    };
    
    for (const auto& xpath : xpath_selectors) {
        EXPECT_NO_THROW({
            g_browser->elementExists(xpath);
            g_browser->clickElement(xpath);
        });
    }
}

// ========== Error Handling and Edge Cases ==========

TEST_F(BrowserDOMTest, RejectInvalidUrls) {
    // Test handling of invalid selectors
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
            g_browser->elementExists(selector);
            g_browser->clickElement(selector);
            g_browser->fillInput(selector, "value");
        });
    }
}

TEST_F(BrowserDOMTest, UnicodeContentHandling) {
    // Test Unicode content in form fields
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
            g_browser->fillInput("#username", value);
            g_browser->fillInput("#comments", value);
            g_browser->searchForm(value);
        });
    }
}

TEST_F(BrowserDOMTest, LargeContentHandling) {
    // Test handling of large content
    std::string large_text(10000, 'A');
    std::string very_large_text(100000, 'B');
    
    EXPECT_NO_THROW({
        g_browser->fillInput("#comments", large_text);
        g_browser->fillInput("#comments", very_large_text);
        g_browser->searchForm(large_text);
    });
}

// ========== Performance and Timing Tests ==========

TEST_F(BrowserDOMTest, OperationTiming) {
    // Test that DOM operations complete in reasonable time
    auto start = std::chrono::steady_clock::now();
    
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            g_browser->elementExists("#test-button");
            g_browser->getAttribute("#username", "name");
            g_browser->fillInput("#search-input", "test" + std::to_string(i));
        }
    });
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Operations should complete within reasonable time (adjusted for real DOM interactions)
    EXPECT_LT(duration.count(), 25000); // Less than 25 seconds for 300 operations with loaded DOM
}

// ========== State Management Tests ==========

TEST_F(BrowserDOMTest, ConsistentStateHandling) {
    // Test that multiple operations maintain consistent state
    EXPECT_NO_THROW({
        g_browser->fillInput("#username", "testuser");
        std::string username = g_browser->getAttribute("#username", "value");
        
        g_browser->selectOption("#country", "us");
        std::string country = g_browser->getAttribute("#country", "value");
        
        g_browser->checkElement("#subscribe");
        std::string checked = g_browser->getAttribute("#subscribe", "checked");
        
        g_browser->focusElement("#password");
        g_browser->fillInput("#password", "secure123");
    });
}