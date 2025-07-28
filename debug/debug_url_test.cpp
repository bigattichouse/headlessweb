#include "src/Browser/Browser.h"
#include <iostream>

int main() {
    try {
        Browser browser;
        std::string test_url = "http://localhost:9876/";
        bool valid = browser.validateUrl(test_url);
        std::cout << "URL: " << test_url << " Valid: " << (valid ? "YES" : "NO") << std::endl;
        
        if (valid) {
            browser.loadUri(test_url);
            std::cout << "Load attempted successfully" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return 0;
}