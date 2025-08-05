#pragma once

#include "Browser/Browser.h"
#include <string>
#include <chrono>
#include <future>

namespace TestUtils {

/**
 * SafePageLoader - A robust page loading utility for tests that need DOM content
 * 
 * This class provides safe page loading that works with the event-driven architecture
 * and prevents the segfaults that occur when tests try to load pages directly.
 */
class SafePageLoader {
public:
    struct LoadResult {
        bool success = false;
        std::string error_message;
        std::string loaded_url;
    };
    
    /**
     * Safely load a page with comprehensive validation and error handling
     */
    static LoadResult loadPageSafely(Browser* browser, const std::string& url, int timeout_ms = 10000);
    
    /**
     * Create a minimal test page and load it safely
     */
    static LoadResult loadMinimalTestPage(Browser* browser, const std::string& html_content);
    
    /**
     * Verify browser is ready for page loading operations
     */
    static bool isBrowserReadyForNavigation(Browser* browser);
    
    /**
     * Wait for page to be completely ready for DOM operations
     */
    static bool waitForPageReady(Browser* browser, int timeout_ms = 5000);
    
private:
    static std::string createDataUrl(const std::string& html_content);
    static bool validateLoadedPage(Browser* browser, const std::string& expected_content);
};

} // namespace TestUtils