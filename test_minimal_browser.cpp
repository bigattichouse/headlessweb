#include <iostream>
#include <chrono>
#include "src/Browser/Browser.h"
#include "src/hweb/Types.h"

int main() {
    std::cout << "Testing minimal Browser creation..." << std::endl;
    
    try {
        auto start = std::chrono::steady_clock::now();
        
        HWeb::HWebConfig test_config;
        std::cout << "Config created" << std::endl;
        
        Browser browser(test_config);
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Browser created in " << elapsed << "ms" << std::endl;
        
        // Try a simple DOM operation
        std::cout << "Testing elementExists..." << std::endl;
        start = std::chrono::steady_clock::now();
        bool exists = browser.elementExists("div");
        end = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "elementExists returned " << exists << " in " << elapsed << "ms" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}