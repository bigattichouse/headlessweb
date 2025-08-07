#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"

extern std::unique_ptr<Browser> g_browser;

class SimpleBrowserDOMTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use EXACTLY the same pattern as working BrowserCoreTest
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("simple_dom_tests");
        
        // CRITICAL FIX: Use global browser instance (properly initialized)
        browser = g_browser.get();
        
        // Create session for browser initialization
        session = std::make_unique<Session>("test_session");
        session->setCurrentUrl("about:blank");
        session->setViewport(1024, 768);
        
        debug_output("SimpleBrowserDOMTest SetUp complete");
    }

    void TearDown() override {
        // Clean up without destroying global browser
        session.reset();
        temp_dir.reset();
    }
    
    Browser* browser;  // Raw pointer to global browser instance
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<Session> session;
};

// Simple test that doesn't load any pages - just like BrowserCoreTest
TEST_F(SimpleBrowserDOMTest, BrowserAccessTest) {
    // Test that browser is accessible without loading pages
    EXPECT_NO_THROW({
        std::string url = browser->getCurrentUrl();
        EXPECT_TRUE(url.empty() || url.find("about:") == 0);
    });
}

// Test basic browser validation methods
TEST_F(SimpleBrowserDOMTest, BasicValidationTest) {
    EXPECT_TRUE(browser->validateUrl("http://example.com"));
    EXPECT_FALSE(browser->validateUrl(""));
}