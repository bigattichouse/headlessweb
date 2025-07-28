#include <iostream>
#include <fstream>
#include <filesystem>
#include "src/Browser/Browser.h"
#include "src/Debug.h"

int main() {
    std::cout << "=== Browser Core Diagnostic Test ===" << std::endl;
    
    try {
        // Create a simple HTML file
        std::string temp_dir = "/tmp/diagnostic_test/";
        std::filesystem::create_directories(temp_dir);
        
        std::string html_content = R"HTML(<!DOCTYPE html>
<html>
<head><title>Diagnostic Test</title></head>
<body>
    <h1 id="heading">Hello World</h1>
    <button id="test-btn" onclick="document.getElementById('result').textContent='Clicked'">Click Me</button>
    <div id="result">Not clicked</div>
</body>
</html>)HTML";
        
        std::string html_file = temp_dir + "test.html";
        std::ofstream file(html_file);
        file << html_content;
        file.close();
        
        std::string file_url = "file://" + html_file;
        std::cout << "Created test file: " << file_url << std::endl;
        
        // Create browser instance
        HWeb::HWebConfig config;
        Browser browser(config);
        
        // Test 1: Basic navigation
        std::cout << "\n--- Test 1: Basic Navigation ---" << std::endl;
        browser.loadUri(file_url);
        bool nav_success = browser.waitForNavigation(5000);
        std::cout << "Navigation success: " << (nav_success ? "YES" : "NO") << std::endl;
        
        // Test 2: Title extraction
        std::cout << "\n--- Test 2: Title Extraction ---" << std::endl;
        std::string title = browser.getPageTitle();
        std::cout << "Page title: '" << title << "'" << std::endl;
        std::cout << "Expected: 'Diagnostic Test'" << std::endl;
        
        // Test 3: JavaScript execution
        std::cout << "\n--- Test 3: JavaScript Execution ---" << std::endl;
        std::string js_title = browser.executeJavascriptSync("return document.title;");
        std::cout << "JS title: '" << js_title << "'" << std::endl;
        
        std::string heading_text = browser.executeJavascriptSync("return document.getElementById('heading').textContent;");
        std::cout << "Heading text: '" << heading_text << "'" << std::endl;
        
        // Test 4: Element existence
        std::cout << "\n--- Test 4: Element Detection ---" << std::endl;
        bool button_exists = browser.elementExists("#test-btn");
        std::cout << "Button exists: " << (button_exists ? "YES" : "NO") << std::endl;
        
        // Test 5: Element interaction
        std::cout << "\n--- Test 5: Element Interaction ---" << std::endl;
        bool click_success = browser.clickElement("#test-btn");
        std::cout << "Click success: " << (click_success ? "YES" : "NO") << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::string result_text = browser.executeJavascriptSync("return document.getElementById('result').textContent;");
        std::cout << "Result text: '" << result_text << "'" << std::endl;
        std::cout << "Expected: 'Clicked'" << std::endl;
        
        // Test 6: Page readiness
        std::cout << "\n--- Test 6: Page State ---" << std::endl;
        std::string ready_state = browser.executeJavascriptSync("return document.readyState;");
        std::cout << "Document ready state: '" << ready_state << "'" << std::endl;
        
        std::string current_url = browser.getCurrentUrl();
        std::cout << "Current URL: '" << current_url << "'" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}