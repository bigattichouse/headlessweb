#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>
#include <chrono>

extern std::unique_ptr<Browser> g_browser;

class DOMEscapingFixesTest : public ::testing::Test {
protected:
    Browser* browser;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("dom_escaping_tests");
        
        // Use global browser instance
        browser = g_browser.get();
        
        // Reset browser to clean state
        browser->loadUri("about:blank");
        browser->waitForNavigation(2000);
        
        // Create and load test page
        createAndLoadTestPage();
    }
    
    void createAndLoadTestPage() {
        std::string test_content = R"(<!DOCTYPE html>
<html>
<head>
    <title>DOM Escaping Test</title>
</head>
<body>
    <form id="test-form">
        <input type="text" id="text-input" name="text-input" />
        <input type="search" id="search-input" name="search-input" placeholder="Search here" />
        <button type="submit" id="submit-btn">Submit</button>
    </form>
    
    <script>
        document.addEventListener('DOMContentLoaded', function() {
            var form = document.getElementById('test-form');
            if (form) {
                form.addEventListener('submit', function(e) {
                    e.preventDefault();
                    var input = document.getElementById('text-input');
                    if (input) {
                        window.lastSubmittedValue = input.value;
                    }
                });
            }
            
            // Mark page as ready for testing
            window.pageReady = true;
        });
    </script>
</body>
</html>)";
        
        // Write test content to file  
        std::string test_file = temp_dir->getPath().string() + "/escaping_test.html";
        std::ofstream file(test_file);
        file << test_content;
        file.close();
        
        // Load the test page
        std::string file_uri = "file://" + test_file;
        browser->loadUri(file_uri);
        browser->waitForNavigation(5000);
        browser->waitForJavaScriptCompletion(2000);
        
        // Wait for page to be ready
        browser->waitForJsCondition("window.pageReady === true", 3000);
    }
};

// ========== FillInput JavaScript Escaping Tests ==========

TEST_F(DOMEscapingFixesTest, FillInputHandlesContractions) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    // Test contraction with apostrophe that previously caused JavaScript errors
    bool result = browser->fillInput("#text-input", "I'm testing contractions");
    EXPECT_TRUE(result);
    
    // Verify the value was set correctly
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "I'm testing contractions");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesSingleQuotes) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Text with 'single quotes' inside");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Text with 'single quotes' inside");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesBackslashes) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Path\\with\\backslashes");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Path\\with\\backslashes");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesMixedQuotesAndBackslashes) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Complex 'string' with\\backslashes and \"quotes\"");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Complex 'string' with\\backslashes and \"quotes\"");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesUnicodeCharacters) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Unicode: é, ñ, test");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Unicode: é, ñ, test");
}

// ========== SearchForm JavaScript Escaping Tests ==========

TEST_F(DOMEscapingFixesTest, SearchFormHandlesContractions) {
    ASSERT_TRUE(browser->waitForSelector("#search-input", 3000));
    
    bool result = browser->searchForm("I'm searching for something");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('search-input').value");
    EXPECT_EQ(value, "I'm searching for something");
}

TEST_F(DOMEscapingFixesTest, SearchFormHandlesSingleQuotes) {
    ASSERT_TRUE(browser->waitForSelector("#search-input", 3000));
    
    bool result = browser->searchForm("Search for 'quoted terms'");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('search-input').value");
    EXPECT_EQ(value, "Search for 'quoted terms'");
}

TEST_F(DOMEscapingFixesTest, SearchFormHandlesBackslashes) {
    ASSERT_TRUE(browser->waitForSelector("#search-input", 3000));
    
    bool result = browser->searchForm("Search\\for\\paths");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('search-input').value");
    EXPECT_EQ(value, "Search\\for\\paths");
}

// ========== Debug Output Tests ==========
// Note: Debug output tests are simplified since debug_output() is controlled by global g_debug flag

// ========== Regression Tests for Previous JavaScript Errors ==========

TEST_F(DOMEscapingFixesTest, NoJavaScriptErrorsWithContractions) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    // Clear any existing JavaScript errors
    browser->executeJavascriptSync("window.jsErrors = [];");
    browser->executeJavascriptSync(R"(
        window.addEventListener('error', function(e) {
            if (!window.jsErrors) window.jsErrors = [];
            window.jsErrors.push(e.message);
        });
    )");
    
    // Fill input with contraction that previously caused errors
    bool result = browser->fillInput("#text-input", "I'm testing for errors");
    EXPECT_TRUE(result);
    
    // Check for JavaScript errors
    std::string errors = browser->executeJavascriptSync("JSON.stringify(window.jsErrors || [])");
    EXPECT_EQ(errors, "[]") << "JavaScript errors detected: " << errors;
}

TEST_F(DOMEscapingFixesTest, NoJavaScriptErrorsWithComplexStrings) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    // Set up error tracking
    browser->executeJavascriptSync("window.jsErrors = [];");
    browser->executeJavascriptSync(R"(
        window.addEventListener('error', function(e) {
            if (!window.jsErrors) window.jsErrors = [];
            window.jsErrors.push(e.message);
        });
    )");
    
    // Test complex string that could cause escaping issues
    std::string complex_string = "Test 'quotes' and\\backslashes and \"double quotes\" and \n newlines";
    bool result = browser->fillInput("#text-input", complex_string);
    EXPECT_TRUE(result);
    
    // Verify no JavaScript errors occurred
    std::string errors = browser->executeJavascriptSync("JSON.stringify(window.jsErrors || [])");
    EXPECT_EQ(errors, "[]") << "JavaScript errors detected with complex string: " << errors;
    
    // Verify the value was set correctly
    std::string value = browser->executeJavascriptSync("document.getElementById('test-input').value");
    EXPECT_EQ(value, complex_string);
}

// ========== Performance and Stability Tests ==========

TEST_F(DOMEscapingFixesTest, EscapingPerformanceWithLargeStrings) {
    ASSERT_TRUE(browser->waitForSelector("#text-input", 3000));
    
    // Create a large string with many characters that need escaping
    std::string large_string;
    for (int i = 0; i < 1000; ++i) {
        large_string += "I'm testing 'quotes' with\\backslashes. ";
    }
    
    auto start = std::chrono::steady_clock::now();
    bool result = browser->fillInput("#text-input", large_string);
    auto end = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(result);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 5000) << "Escaping should complete within 5 seconds";
    
    // Verify the value was set correctly (check a substring to avoid huge output)
    std::string value = browser->executeJavascriptSync("document.getElementById('test-input').value.substring(0, 100)");
    EXPECT_EQ(value, large_string.substr(0, 100));
}