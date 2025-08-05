#include "SafePageLoader.h"
#include "Debug.h"
#include <thread>
#include <sstream>
#include <algorithm>

namespace TestUtils {

SafePageLoader::LoadResult SafePageLoader::loadPageSafely(Browser* browser, const std::string& url, int timeout_ms) {
    LoadResult result;
    
    try {
        // Step 1: Comprehensive browser validation
        if (!browser) {
            result.error_message = "Browser is null";
            return result;
        }
        
        if (!browser->isObjectValid()) {
            result.error_message = "Browser object is not valid";
            return result;
        }
        
        // Step 2: Validate URL
        if (!browser->validateUrl(url)) {
            result.error_message = "Invalid URL: " + url;
            return result;
        }
        
        // Step 3: Check event-driven infrastructure readiness
        auto event_bus = browser->getEventBus();
        if (!event_bus) {
            result.error_message = "Browser event infrastructure not ready";
            return result;
        }
        
        debug_output("SafePageLoader: Starting navigation to " + url);
        
        // Step 4: Perform navigation with comprehensive error handling
        browser->loadUri(url);
        
        // Step 5: Wait for navigation with timeout
        bool nav_success = browser->waitForNavigation(timeout_ms);
        if (!nav_success) {
            result.error_message = "Navigation timeout after " + std::to_string(timeout_ms) + "ms";
            return result;
        }
        
        // Step 6: Wait for page readiness
        if (!waitForPageReady(browser, 3000)) {
            result.error_message = "Page readiness timeout";
            return result;
        }
        
        // Step 7: Validate successful load
        std::string current_url = browser->getCurrentUrl();
        if (current_url.empty()) {
            result.error_message = "No URL loaded after navigation";
            return result;
        }
        
        result.success = true;
        result.loaded_url = current_url;
        debug_output("SafePageLoader: Successfully loaded " + current_url);
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_message = "Exception during page loading: " + std::string(e.what());
        return result;
    } catch (...) {
        result.error_message = "Unknown exception during page loading";
        return result;
    }
}

SafePageLoader::LoadResult SafePageLoader::loadMinimalTestPage(Browser* browser, const std::string& html_content) {
    // Create a data URL for the HTML content
    std::string data_url = createDataUrl(html_content);
    return loadPageSafely(browser, data_url, 8000);
}

bool SafePageLoader::isBrowserReadyForNavigation(Browser* browser) {
    if (!browser || !browser->isObjectValid()) {
        return false;
    }
    
    // Check if event-driven infrastructure is available
    return browser->getEventBus() != nullptr;
}

bool SafePageLoader::waitForPageReady(Browser* browser, int timeout_ms) {
    if (!browser) return false;
    
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    try {
        // Use event-driven readiness detection if available
        auto readiness_tracker = browser->getReadinessTracker();
        if (readiness_tracker) {
            auto readiness_future = readiness_tracker->waitForBasicReadiness(timeout_ms);
            auto status = readiness_future.wait_for(timeout);
            
            if (status == std::future_status::ready) {
                return readiness_future.get();
            } else {
                debug_output("SafePageLoader: Readiness tracker timeout");
                return false;
            }
        }
        
        // Fallback: Basic JavaScript readiness check
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            try {
                std::string ready_state = browser->executeJavascriptSyncSafe("document.readyState");
                if (ready_state == "complete" || ready_state == "interactive") {
                    // Additional pause for stability
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    return true;
                }
            } catch (...) {
                // JavaScript execution failed, page might not be ready
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        debug_output("SafePageLoader: Page readiness timeout");
        return false;
        
    } catch (const std::exception& e) {
        debug_output("SafePageLoader: Exception in waitForPageReady: " + std::string(e.what()));
        return false;
    }
}

std::string SafePageLoader::createDataUrl(const std::string& html_content) {
    // Create a data URL with proper HTML structure
    std::string complete_html = html_content;
    
    // Ensure minimal HTML structure if not present
    if (complete_html.find("<!DOCTYPE") == std::string::npos) {
        std::ostringstream oss;
        oss << "<!DOCTYPE html><html><head><title>Test Page</title></head><body>";
        oss << complete_html;
        oss << "</body></html>";
        complete_html = oss.str();
    }
    
    return "data:text/html," + complete_html;
}

bool SafePageLoader::validateLoadedPage(Browser* browser, const std::string& expected_content) {
    if (!browser) return false;
    
    try {
        // Check if page contains expected content
        std::string page_source = browser->getPageSource();
        return page_source.find(expected_content) != std::string::npos;
    } catch (...) {
        return false;
    }
}

} // namespace TestUtils