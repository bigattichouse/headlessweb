#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <memory>
#include <chrono>

extern std::unique_ptr<Browser> g_browser;

class SPANavigationValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("spa_navigation_validation_tests");
        browser_ = g_browser.get();
        browser_->loadUri("about:blank");
        browser_->waitForNavigation(2000);
    }
    
    std::string executeWrappedJS(const std::string& jsCode) {
        std::string wrapped = "(function() { " + jsCode + " })()";
        return browser_->executeJavascriptSync(wrapped);
    }
    
    void setupSPATestPage() {
        std::string spa_html = R"HTMLDELIM(
            <html><body>
                <h1>SPA Navigation Test</h1>
                <div id="status">Ready</div>
                <div id="current-route">home</div>
                <button onclick="navigateHash()">Hash Navigation</button>
                <button onclick="navigatePushState()">PushState Navigation</button>
                
                <script>
                    function navigateHash() {
                        window.location.hash = '#dashboard-spa';
                        document.getElementById('status').textContent = 'Hash navigation triggered';
                        document.getElementById('current-route').textContent = 'dashboard';
                    }
                    
                    function navigatePushState() {
                        window.history.pushState({}, '', '/app/dashboard');
                        document.getElementById('status').textContent = 'PushState navigation triggered';
                        document.getElementById('current-route').textContent = 'dashboard';
                    }
                    
                    // Helper function for setTimeout navigation
                    function changeHash(route) {
                        window.location.hash = '#' + route;
                        document.getElementById('current-route').textContent = route;
                    }
                    
                    // Helper function for delayed pushState
                    function delayedPushState(path, delay) {
                        setTimeout(() => {
                            window.history.pushState({}, '', path);
                            document.getElementById('status').textContent = 'Delayed PushState to ' + path;
                            // Extract route from path
                            var route = path.split('/').pop();
                            document.getElementById('current-route').textContent = route;
                        }, delay);
                    }
                </script>
            </body></html>
        )HTMLDELIM";
        
        auto html_file = temp_dir->createFile("spa_test.html", spa_html);
        std::string file_url = "file://" + html_file.string();
        
        browser_->loadUri(file_url);
        browser_->waitForNavigation(3000);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Verify page loaded
        ASSERT_TRUE(browser_->elementExists("#status"));
        ASSERT_TRUE(browser_->elementExists("#current-route"));
    }
    
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    Browser* browser_;
};

TEST_F(SPANavigationValidationTest, ComprehensiveSPANavigationTest) {
    debug_output("Starting Comprehensive SPA Navigation Test");
    
    setupSPATestPage();
    
    std::cout << "\n=== COMPREHENSIVE SPA NAVIGATION ANALYSIS ===" << std::endl;
    
    // Test 1: Hash-based navigation (current working implementation)
    std::cout << "\n--- Test 1: Hash Navigation (Current Implementation) ---" << std::endl;
    executeWrappedJS("setTimeout(() => window.location.hash = '#dashboard-spa', 300);");
    
    bool hash_result = browser_->waitForSPANavigation("dashboard", 2000);
    std::cout << "Hash navigation result: " << (hash_result ? "SUCCESS" : "FAILURE") << std::endl;
    
    if (hash_result) {
        std::string current_url = browser_->getCurrentUrl();
        std::cout << "Current URL after hash navigation: " << current_url << std::endl;
        EXPECT_TRUE(current_url.find("dashboard") != std::string::npos) 
            << "URL should contain 'dashboard': " << current_url;
    }
    
    // Reset page state
    executeWrappedJS("window.location.hash = ''; document.getElementById('current-route').textContent = 'home';");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test 2: History.pushState navigation (original requirement that was changed)
    std::cout << "\n--- Test 2: History.pushState Navigation (Original Requirement) ---" << std::endl;
    executeWrappedJS("setTimeout(() => window.history.pushState({}, '', '/app/dashboard'), 300);");
    
    bool pushstate_result = browser_->waitForSPANavigation("dashboard", 2000);
    std::cout << "PushState navigation result: " << (pushstate_result ? "SUCCESS" : "FAILURE") << std::endl;
    
    if (pushstate_result) {
        std::string current_url = browser_->getCurrentUrl();
        std::cout << "Current URL after pushState navigation: " << current_url << std::endl;
    } else {
        // Investigate why pushState failed
        std::string current_url = browser_->getCurrentUrl();
        std::string pathname_check = executeWrappedJS("return window.location.pathname;");
        std::string href_check = executeWrappedJS("return window.location.href;");
        
        std::cout << "PushState failure analysis:" << std::endl;
        std::cout << "  - Current URL (C++): " << current_url << std::endl;
        std::cout << "  - window.location.pathname: " << pathname_check << std::endl;
        std::cout << "  - window.location.href: " << href_check << std::endl;
        
        // Check if the route is actually there but waitForSPANavigation can't detect it
        std::string manual_check = executeWrappedJS(R"(
            var path = window.location.pathname;
            var href = window.location.href;
            var route = 'dashboard';
            return (path.indexOf(route) !== -1 || href.indexOf(route) !== -1);
        )");
        std::cout << "  - Manual route detection: " << manual_check << std::endl;
    }
    
    // Reset for next test
    executeWrappedJS("window.history.pushState({}, '', '/'); document.getElementById('current-route').textContent = 'home';");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test 3: Verify both patterns can be detected (ideal scenario)
    std::cout << "\n--- Test 3: Dual Pattern Detection Analysis ---" << std::endl;
    
    // Test if waitForSPANavigation can handle both types in sequence
    bool dual_support = true;
    
    // First test hash again to ensure consistency
    executeWrappedJS("setTimeout(() => window.location.hash = '#profile-spa', 200);");
    bool hash_second = browser_->waitForSPANavigation("profile", 1500);
    std::cout << "Hash navigation (second test): " << (hash_second ? "SUCCESS" : "FAILURE") << std::endl;
    
    if (!hash_second) {
        dual_support = false;
    }
    
    // Reset and test pushState again
    executeWrappedJS("window.location.hash = ''; window.history.pushState({}, '', '/');");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    executeWrappedJS("setTimeout(() => window.history.pushState({}, '', '/app/settings'), 200);");
    bool pushstate_second = browser_->waitForSPANavigation("settings", 1500);
    std::cout << "PushState navigation (second test): " << (pushstate_second ? "SUCCESS" : "FAILURE") << std::endl;
    
    if (!pushstate_second) {
        dual_support = false;
    }
    
    // Analysis and recommendations
    std::cout << "\n=== ANALYSIS AND RECOMMENDATIONS ===" << std::endl;
    
    if (hash_result && pushstate_result) {
        std::cout << "✅ EXCELLENT: Both hash and pushState navigation work perfectly" << std::endl;
        std::cout << "✅ The original test change was UNNECESSARY" << std::endl;
        std::cout << "✅ Recommendation: Restore original pushState test or add both variants" << std::endl;
        EXPECT_TRUE(true) << "Both navigation types work - test change was unnecessary";
    } else if (hash_result && !pushstate_result) {
        std::cout << "🟡 PARTIAL: Only hash navigation works reliably" << std::endl;
        std::cout << "🟡 The original test change was JUSTIFIED" << std::endl;
        std::cout << "🟡 Recommendation: Keep hash-based test but investigate pushState support" << std::endl;
        EXPECT_TRUE(hash_result) << "Hash navigation should continue to work";
    } else if (!hash_result && pushstate_result) {
        std::cout << "⚠️ UNEXPECTED: Only pushState works (hash navigation broke)" << std::endl;
        std::cout << "⚠️ This suggests our fix may have introduced a regression" << std::endl;
        EXPECT_TRUE(pushstate_result) << "PushState should work";
    } else {
        std::cout << "🔴 CRITICAL: Neither navigation type works reliably" << std::endl;
        std::cout << "🔴 This indicates a fundamental problem with waitForSPANavigation" << std::endl;
        FAIL() << "Both navigation types failed - fundamental SPA support broken";
    }
    
    // Performance comparison
    std::cout << "\n=== PERFORMANCE COMPARISON ===" << std::endl;
    std::cout << "Hash navigation: " << (hash_result ? "✅ Reliable" : "❌ Unreliable") << std::endl;
    std::cout << "PushState navigation: " << (pushstate_result ? "✅ Reliable" : "❌ Unreliable") << std::endl;
    std::cout << "Dual pattern support: " << (dual_support ? "✅ Supported" : "❌ Inconsistent") << std::endl;
}

TEST_F(SPANavigationValidationTest, WaitForSPANavigationInternalAnalysis) {
    debug_output("Starting SPA Navigation Internal Analysis");
    
    setupSPATestPage();
    
    std::cout << "\n=== INTERNAL MECHANISM ANALYSIS ===" << std::endl;
    
    // Test the internal detection mechanisms used by waitForSPANavigation
    
    // Test 1: URL change detection
    std::string initial_url = browser_->getCurrentUrl();
    std::cout << "Initial URL: " << initial_url << std::endl;
    
    // Trigger hash change and check if getCurrentUrl() detects it
    executeWrappedJS("window.location.hash = '#test-route';");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string url_after_hash = browser_->getCurrentUrl();
    bool url_change_detected = (url_after_hash != initial_url);
    std::cout << "URL after hash change: " << url_after_hash << std::endl;
    std::cout << "URL change detected by getCurrentUrl(): " << (url_change_detected ? "YES" : "NO") << std::endl;
    
    // Reset
    executeWrappedJS("window.location.hash = '';");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test 2: PushState URL detection
    executeWrappedJS("window.history.pushState({}, '', '/test-pushstate');");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string url_after_pushstate = browser_->getCurrentUrl();
    bool pushstate_url_detected = (url_after_pushstate != initial_url);
    std::cout << "URL after pushState: " << url_after_pushstate << std::endl;
    std::cout << "PushState change detected by getCurrentUrl(): " << (pushstate_url_detected ? "YES" : "NO") << std::endl;
    
    // Test 3: JavaScript-based detection (what waitForSPANavigation uses internally)
    executeWrappedJS("window.history.pushState({}, '', '/js-detection-test');");
    
    std::string js_detection_result = executeWrappedJS(R"(
        var path = window.location.pathname;
        var href = window.location.href;
        var route = 'detection';
        var hash = window.location.hash;
        return JSON.stringify({
            pathname: path,
            href: href,
            hash: hash,
            contains_route: (path.indexOf(route) !== -1 || href.indexOf(route) !== -1 || hash.indexOf(route) !== -1)
        });
    )");
    
    std::cout << "JavaScript detection result: " << js_detection_result << std::endl;
    
    // Analysis of detection mechanisms
    std::cout << "\n=== DETECTION MECHANISM ANALYSIS ===" << std::endl;
    
    if (url_change_detected && pushstate_url_detected) {
        std::cout << "✅ Both hash and pushState changes are detected by getCurrentUrl()" << std::endl;
        std::cout << "✅ waitForSPANavigation should work for both patterns" << std::endl;
    } else if (url_change_detected && !pushstate_url_detected) {
        std::cout << "🟡 Only hash changes detected by getCurrentUrl()" << std::endl;
        std::cout << "🟡 waitForSPANavigation relies on JavaScript detection for pushState" << std::endl;
    } else if (!url_change_detected && pushstate_url_detected) {
        std::cout << "⚠️ Only pushState changes detected by getCurrentUrl()" << std::endl;
        std::cout << "⚠️ This is unusual - hash changes should be detected" << std::endl;
    } else {
        std::cout << "🔴 Neither hash nor pushState changes detected by getCurrentUrl()" << std::endl;
        std::cout << "🔴 waitForSPANavigation relies entirely on JavaScript detection" << std::endl;
    }
    
    // Verify that our test modifications actually work as expected
    EXPECT_TRUE(url_change_detected || pushstate_url_detected) 
        << "At least one navigation type should be detectable by URL change";
}