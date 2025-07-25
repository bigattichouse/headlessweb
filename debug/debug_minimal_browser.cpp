#include <iostream>
#include <chrono>
#include <thread>
#include "../src/Browser/Browser.h"

using namespace std::chrono_literals;

int main() {
    std::cout << "=== Minimal Browser Debug Test ===" << std::endl;
    
    try {
        std::cout << "Creating browser..." << std::endl;
        HWeb::HWebConfig config;
        Browser browser(config);
        std::cout << "✓ Browser created" << std::endl;
        
        // Test 1: Very simple page
        std::cout << "\n=== Test 1: Simple Page Load ===" << std::endl;
        std::string simple_html = "data:text/html,<html><head><title>Simple</title></head><body><h1>Test</h1></body></html>";
        browser.loadUri(simple_html);
        
        // Use proper page ready waiting instead of sleep
        browser.waitForPageReadyEvent(5000);
        
        std::string title = browser.executeJavascriptSync("document.title");
        std::cout << "Title: '" << title << "'" << std::endl;
        if (title == "Simple") {
            std::cout << "✓ PASS: Title retrieved correctly" << std::endl;
        } else {
            std::cout << "✗ FAIL: Expected 'Simple', got '" << title << "'" << std::endl;
        }
        
        // Test 2: JavaScript execution
        std::cout << "\n=== Test 2: JavaScript Execution ===" << std::endl;
        std::string result = browser.executeJavascriptSync("2 + 2");
        std::cout << "2 + 2 = '" << result << "'" << std::endl;
        if (result == "4") {
            std::cout << "✓ PASS: JavaScript execution working" << std::endl;
        } else {
            std::cout << "✗ FAIL: Expected '4', got '" << result << "'" << std::endl;
        }
        
        // Test 3: Element existence
        std::cout << "\n=== Test 3: Element Existence ===" << std::endl;
        bool h1_exists = browser.elementExists("h1");
        std::cout << "h1 exists: " << (h1_exists ? "YES" : "NO") << std::endl;
        if (h1_exists) {
            std::cout << "✓ PASS: Element detection working" << std::endl;
        } else {
            std::cout << "✗ FAIL: h1 element should exist" << std::endl;
        }
        
        // Test 4: Current URL
        std::cout << "\n=== Test 4: URL Management ===" << std::endl;
        std::string current_url = browser.getCurrentUrl();
        std::cout << "Current URL: '" << current_url << "'" << std::endl;
        if (current_url.find("data:text/html") == 0) {
            std::cout << "✓ PASS: URL management working" << std::endl;
        } else {
            std::cout << "✗ FAIL: Expected data: URL" << std::endl;
        }
        
        // Test 5: Page source
        std::cout << "\n=== Test 5: Page Source ===" << std::endl;
        std::string source = browser.getPageSource();
        std::cout << "Page source length: " << source.length() << " characters" << std::endl;
        if (source.find("<h1>Test</h1>") != std::string::npos) {
            std::cout << "✓ PASS: Page source contains expected content" << std::endl;
        } else {
            std::cout << "✗ FAIL: Page source missing expected content" << std::endl;
            std::cout << "Source preview: '" << source.substr(0, 100) << "...'" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Minimal browser test completed ===" << std::endl;
    return 0;
}