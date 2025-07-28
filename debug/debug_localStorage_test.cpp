#include <iostream>
#include "src/Browser/Browser.h"
#include <thread>
#include <chrono>

int main() {
    HWeb::HWebConfig config;
    Browser browser(config);
    
    // Test 1: Simple HTML page to check localStorage basics
    std::string simple_html = R"(
<!DOCTYPE html>
<html>
<head><title>LocalStorage Test</title></head>
<body>
    <h1>Storage Test</h1>
    <script>
        console.log("Script starting");
        try {
            localStorage.setItem("test", "value");
            console.log("localStorage set successful");
        } catch(e) {
            console.log("localStorage set failed:", e.message);
        }
    </script>
</body>
</html>
)";
    
    std::string data_url = "data:text/html," + simple_html;
    
    std::cout << "Loading data URL..." << std::endl;
    browser.loadUri(data_url);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    std::cout << "Testing localStorage access..." << std::endl;
    
    // Direct JavaScript test
    std::string js_test = R"(
        try {
            localStorage.setItem('direct_test', 'direct_value');
            var result = localStorage.getItem('direct_test');
            'SUCCESS: ' + result;
        } catch(e) {
            'ERROR: ' + e.message;
        }
    )";
    
    std::string result = browser.executeJavascriptSync(js_test);
    std::cout << "Direct JS result: " << result << std::endl;
    
    // Test via Browser storage methods
    std::map<std::string, std::string> test_storage = {{"key1", "value1"}};
    browser.setLocalStorage(test_storage);
    
    auto retrieved = browser.getLocalStorage();
    std::cout << "Browser method result size: " << retrieved.size() << std::endl;
    for (const auto& [key, value] : retrieved) {
        std::cout << "  " << key << " = " << value << std::endl;
    }
    
    return 0;
}