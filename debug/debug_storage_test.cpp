#include <iostream>
#include <chrono>
#include <thread>
#include "../src/Browser/Browser.h"

using namespace std::chrono_literals;

int main() {
    std::cout << "=== Browser Storage Debug Test ===" << std::endl;
    
    try {
        // Create browser instance
        HWeb::HWebConfig config;
        Browser browser(config);
        
        std::cout << "✓ Browser created successfully" << std::endl;
        
        // Test 1: Load simple page with storage functions
        std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>Storage Test</title></head>
<body>
    <h1>Storage Test</h1>
    <script>
        function testStorage() {
            console.log('Testing storage functions...');
            localStorage.setItem('test', 'value');
            return localStorage.getItem('test');
        }
    </script>
</body>
</html>
)HTML";
        
        std::string data_url = "data:text/html;charset=utf-8," + test_html;
        std::cout << "Loading test page..." << std::endl;
        browser.loadUri(data_url);
        
        // Use proper page ready waiting instead of sleep
        bool page_ready = browser.waitForPageReadyEvent(5000);
        std::cout << "✓ Page loaded (ready: " << (page_ready ? "YES" : "NO") << ")" << std::endl;
        
        // DEBUG: Check individual JavaScript components
        std::string localStorage_check = browser.executeJavascriptSync("typeof localStorage");
        std::cout << "DEBUG: localStorage type: '" << localStorage_check << "'" << std::endl;
        
        std::string window_localStorage_check = browser.executeJavascriptSync("typeof window.localStorage");
        std::cout << "DEBUG: window.localStorage type: '" << window_localStorage_check << "'" << std::endl;
        
        std::string function_check = browser.executeJavascriptSync("typeof testStorage");
        std::cout << "DEBUG: testStorage type: '" << function_check << "'" << std::endl;
        
        std::string readyState_check = browser.executeJavascriptSync("document.readyState");
        std::cout << "DEBUG: document ready state: '" << readyState_check << "'" << std::endl;
        
        // Try to access localStorage properties to see what happens
        std::string localStorage_error = browser.executeJavascriptSync(
            "(function() { "
            "  try { "
            "    var test = localStorage; "
            "    return 'accessible'; "
            "  } catch(e) { "
            "    return 'error: ' + e.message; "
            "  } "
            "})()");
        std::cout << "DEBUG: localStorage access: '" << localStorage_error << "'" << std::endl;
        
        // DEBUG: Test the exact readiness check logic
        std::string readiness_test = browser.executeJavascriptSync(
            "(function() { "
            "  try { "
            "    // Basic checks"
            "    if (typeof document === 'undefined' || typeof window === 'undefined') return 'fail_basic'; "
            "    if (document.readyState !== 'complete') return 'fail_ready_state'; "
            "    "
            "    // Test that script execution works by defining and calling a test function"
            "    window.testScriptExecution = function() { return 'working'; }; "
            "    var result = window.testScriptExecution(); "
            "    delete window.testScriptExecution; "
            "    if (result !== 'working') return 'fail_script_exec'; "
            "    "
            "    // Test localStorage if available (may be blocked for data: URLs)"
            "    var localStorage_works = true; "
            "    try { "
            "      localStorage.setItem('__hweb_test__', 'test'); "
            "      var stored = localStorage.getItem('__hweb_test__'); "
            "      localStorage.removeItem('__hweb_test__'); "
            "      localStorage_works = (stored === 'test'); "
            "    } catch(e) { "
            "      // localStorage may be blocked for data: URLs - this is normal"
            "      localStorage_works = true; "
            "    } "
            "    "
            "    return result === 'working' && localStorage_works ? 'pass' : 'fail_final'; "
            "  } catch(e) { "
            "    return 'error: ' + e.message; "
            "  } "
            "})()");
        std::cout << "DEBUG: readiness test result: '" << readiness_test << "'" << std::endl;
        
        // Test 2: Check basic JavaScript execution
        std::string title = browser.executeJavascriptSync("document.title");
        std::cout << "Page title: '" << title << "'" << std::endl;
        
        // Test 3: Test localStorage
        std::cout << "\n=== Testing localStorage ===" << std::endl;
        std::string storage_result = browser.executeJavascriptSync("testStorage()");
        std::cout << "localStorage test result: '" << storage_result << "'" << std::endl;
        
        // Test 4: Direct localStorage operations
        browser.executeJavascriptSync("localStorage.setItem('debug', 'test123')");
        std::string debug_value = browser.executeJavascriptSync("localStorage.getItem('debug')");
        std::cout << "Direct localStorage get: '" << debug_value << "'" << std::endl;
        
        // Test 5: Cookie operations
        std::cout << "\n=== Testing Cookies ===" << std::endl;
        Cookie test_cookie;
        test_cookie.name = "debug_cookie";
        test_cookie.value = "debug_value";
        test_cookie.path = "/";
        
        browser.setCookie(test_cookie);
        std::cout << "Cookie set command executed" << std::endl;
        
        // Wait a bit for cookie to be processed
        std::this_thread::sleep_for(500ms);
        
        // Use async callback to get cookies
        bool cookies_received = false;
        std::vector<Cookie> cookies;
        
        browser.getCookiesAsync([&cookies_received, &cookies](std::vector<Cookie> retrieved_cookies) {
            cookies = std::move(retrieved_cookies);
            cookies_received = true;
        });
        
        // Wait for async callback
        for (int i = 0; i < 50 && !cookies_received; i++) {
            std::this_thread::sleep_for(100ms);
        }
        
        if (cookies_received) {
            std::cout << "Retrieved " << cookies.size() << " cookies" << std::endl;
            for (const auto& cookie : cookies) {
                std::cout << "  Cookie: " << cookie.name << " = " << cookie.value << std::endl;
            }
        } else {
            std::cout << "Timeout waiting for cookies" << std::endl;
        }
        
        // Test 6: Direct cookie access via JavaScript
        std::string js_cookies = browser.executeJavascriptSync("document.cookie");
        std::cout << "JavaScript cookies: '" << js_cookies << "'" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Debug test completed ===" << std::endl;
    return 0;
}