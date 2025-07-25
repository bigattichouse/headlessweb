#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include "../src/Browser/Browser.h"

using namespace std::chrono_literals;

// Signal handler for catching segfaults
void segfault_handler(int sig) {
    std::cout << "\n!!! SEGFAULT DETECTED in events test !!!" << std::endl;
    std::cout << "Signal: " << sig << std::endl;
    exit(1);
}

int main() {
    std::cout << "=== Browser Events Debug Test ===" << std::endl;
    
    // Install segfault handler
    signal(SIGSEGV, segfault_handler);
    
    try {
        // Create browser instance
        HWeb::HWebConfig config;
        Browser browser(config);
        
        std::cout << "✓ Browser created successfully" << std::endl;
        
        // Test 1: Load simple page
        std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>Events Test</title></head>
<body>
    <h1>Events Test</h1>
    <button id="test-btn">Test Button</button>
    <div id="status">Ready</div>
</body>
</html>
)HTML";
        
        std::string data_url = "data:text/html;charset=utf-8," + test_html;
        std::cout << "Loading test page..." << std::endl;
        browser.loadUri(data_url);
        
        // Wait for page load
        std::this_thread::sleep_for(1000ms);
        std::cout << "✓ Page loaded" << std::endl;
        
        // Test 2: Basic page ready check
        std::string title = browser.executeJavascriptSync("document.title");
        std::cout << "Page title: '" << title << "'" << std::endl;
        
        if (title.empty()) {
            std::cout << "WARNING: Page title is empty - page may not be fully loaded" << std::endl;
        }
        
        // Test 3: Test waitForPageReady (this might be causing segfaults)
        std::cout << "\n=== Testing waitForPageReady ===" << std::endl;
        Session test_session("debug_session");
        test_session.setCurrentUrl(data_url);
        
        std::cout << "Calling waitForPageReady..." << std::endl;
        bool page_ready = browser.waitForPageReady(test_session);
        std::cout << "waitForPageReady result: " << (page_ready ? "SUCCESS" : "FAILED") << std::endl;
        
        // Test 4: Test navigation waiting (another potential segfault source)
        std::cout << "\n=== Testing navigation waiting ===" << std::endl;
        std::cout << "Calling waitForNavigation with 1000ms timeout..." << std::endl;
        bool nav_result = browser.waitForNavigation(1000);
        std::cout << "waitForNavigation result: " << (nav_result ? "SUCCESS" : "TIMEOUT") << std::endl;
        
        // Test 5: Element waiting
        std::cout << "\n=== Testing element waiting ===" << std::endl;
        std::cout << "Calling waitForSelector for #test-btn..." << std::endl;
        bool element_ready = browser.waitForSelector("#test-btn", 1000);
        std::cout << "waitForSelector result: " << (element_ready ? "SUCCESS" : "TIMEOUT") << std::endl;
        
        // Test 6: Event loop manager state
        std::cout << "\n=== Testing EventLoopManager state ===" << std::endl;
        // We can't directly access private members, but we can test behavior
        
        std::cout << "Testing multiple wait calls..." << std::endl;
        for (int i = 0; i < 3; i++) {
            std::cout << "  Wait iteration " << (i+1) << std::endl;
            bool result = browser.waitForSelector("#status", 100);
            std::cout << "    Result: " << (result ? "SUCCESS" : "TIMEOUT") << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Events debug test completed successfully ===" << std::endl;
    return 0;
}