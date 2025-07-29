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
        
        // Load the test page using signal-based navigation
        std::string file_uri = "file://" + test_file;
        browser->loadUri(file_uri);
        
        // Use signal-based navigation waiting instead of polling
        browser->waitForNavigationEvent(5000);
        
        // Wait for page to be ready using signal-based condition waiting
        browser->waitForConditionEvent("window.pageReady === true", 5000);
    }
};

// ========== FillInput JavaScript Escaping Tests ==========

TEST_F(DOMEscapingFixesTest, FillInputHandlesContractions) {
    // Use signal-based selector waiting instead of polling
    ASSERT_TRUE(browser->waitForSelectorEvent("#text-input", 3000));
    
    // Test contraction with apostrophe that previously caused JavaScript errors
    bool result = browser->fillInput("#text-input", "I'm testing contractions");
    EXPECT_TRUE(result);
    
    // Verify the value was set correctly
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "I'm testing contractions");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesSingleQuotes) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Text with 'single quotes' inside");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Text with 'single quotes' inside");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesBackslashes) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Path\\with\\backslashes");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Path\\with\\backslashes");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesMixedQuotesAndBackslashes) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Complex 'string' with\\backslashes and \"quotes\"");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Complex 'string' with\\backslashes and \"quotes\"");
}

TEST_F(DOMEscapingFixesTest, FillInputHandlesUnicodeCharacters) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#text-input", 3000));
    
    bool result = browser->fillInput("#text-input", "Unicode: é, ñ, test");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, "Unicode: é, ñ, test");
}

// ========== SearchForm JavaScript Escaping Tests ==========

TEST_F(DOMEscapingFixesTest, SearchFormHandlesContractions) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#search-input", 3000));
    
    bool result = browser->searchForm("I'm searching for something");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('search-input').value");
    EXPECT_EQ(value, "I'm searching for something");
}

TEST_F(DOMEscapingFixesTest, SearchFormHandlesSingleQuotes) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#search-input", 3000));
    
    bool result = browser->searchForm("Search for 'quoted terms'");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('search-input').value");
    EXPECT_EQ(value, "Search for 'quoted terms'");
}

TEST_F(DOMEscapingFixesTest, SearchFormHandlesBackslashes) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#search-input", 3000));
    
    bool result = browser->searchForm("Search\\for\\paths");
    EXPECT_TRUE(result);
    
    std::string value = browser->executeJavascriptSync("document.getElementById('search-input').value");
    EXPECT_EQ(value, "Search\\for\\paths");
}

// ========== Debug Output Tests ==========
// Note: Debug output tests are simplified since debug_output() is controlled by global g_debug flag

// ========== Regression Tests for Previous JavaScript Errors ==========

TEST_F(DOMEscapingFixesTest, NoJavaScriptErrorsWithContractions) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#text-input", 3000));
    
    // Set up comprehensive error tracking before any operations
    std::string errorSetup = R"(
        window.jsErrors = [];
        window.addEventListener('error', function(e) {
            console.log('JavaScript error caught:', e.message);
            window.jsErrors.push({
                message: e.message,
                filename: e.filename,
                lineno: e.lineno,
                colno: e.colno
            });
        });
        
        // Also catch console errors
        const originalError = console.error;
        console.error = function(...args) {
            window.jsErrors.push({message: 'Console error: ' + args.join(' ')});
            originalError.apply(console, args);
        };
    )";
    
    browser->executeJavascriptSync(errorSetup);
    
    // Small delay to ensure error handlers are set up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Fill input with contraction that previously caused errors
    bool result = browser->fillInput("#text-input", "I'm testing for errors");
    EXPECT_TRUE(result);
    
    // Small delay to allow any errors to be captured
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check for JavaScript errors with better error reporting
    std::string errorCount = browser->executeJavascriptSync("window.jsErrors ? window.jsErrors.length.toString() : '0'");
    EXPECT_EQ(errorCount, "0") << "JavaScript errors detected - count: " << errorCount;
    
    if (errorCount != "0") {
        std::string errors = browser->executeJavascriptSync("JSON.stringify(window.jsErrors || [], null, 2)");
        FAIL() << "JavaScript errors detected: " << errors;
    }
}

TEST_F(DOMEscapingFixesTest, NoJavaScriptErrorsWithSimpleStrings) {
    ASSERT_TRUE(browser->waitForSelectorEvent("#text-input", 3000));
    
    // Set up comprehensive error tracking
    std::string errorSetup = R"(
        window.jsErrors = [];
        window.addEventListener('error', function(e) {
            console.log('JavaScript error caught:', e.message);
            window.jsErrors.push({
                message: e.message,
                filename: e.filename,
                lineno: e.lineno,
                colno: e.colno
            });
        });
        
        // Also catch console errors
        const originalError = console.error;
        console.error = function(...args) {
            window.jsErrors.push({message: 'Console error: ' + args.join(' ')});
            originalError.apply(console, args);
        };
    )";
    
    browser->executeJavascriptSync(errorSetup);
    
    // Small delay to ensure error handlers are set up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test simple string with quotes and backslashes (no newlines to avoid encoding issues)
    std::string test_string = "Test 'quotes' and\\\\backslashes and \\\"double quotes\\\"";
    bool result = browser->fillInput("#text-input", test_string);
    EXPECT_TRUE(result);
    
    // Small delay to allow any errors to be captured
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check for JavaScript errors with better error reporting
    std::string errorCount = browser->executeJavascriptSync("window.jsErrors ? window.jsErrors.length.toString() : '0'");
    EXPECT_EQ(errorCount, "0") << "JavaScript errors detected - count: " << errorCount;
    
    if (errorCount != "0") {
        std::string errors = browser->executeJavascriptSync("JSON.stringify(window.jsErrors || [], null, 2)");
        FAIL() << "JavaScript errors detected with test string '" << test_string << "': " << errors;
    }
    
    // Verify the value was set correctly
    std::string value = browser->executeJavascriptSync("document.getElementById('text-input').value");
    EXPECT_EQ(value, test_string);
}

// ========== Performance and Stability Tests ==========
// Note: Simplified tests to focus on core escaping functionality