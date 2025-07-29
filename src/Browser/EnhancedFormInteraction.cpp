#include "Browser.h"
#include "../Debug.h"
#include <thread>
#include <chrono>

// Enhanced form interaction specifically for modern dynamic forms like Google
bool Browser::fillInputEnhanced(const std::string& selector, const std::string& value) {
    debug_output("Enhanced form interaction for selector: " + selector);
    
    // Step 1: Wait for element with longer timeout for dynamic content
    if (!waitForSelectorEvent(selector, 3000)) {
        debug_output("Enhanced fill: Element not found within timeout");
        return false;
    }
    
    // Step 2: Escape the value and selector for JavaScript
    std::string escaped_value = value;
    std::string escaped_selector = selector;
    
    // Helper function to escape strings for JavaScript
    auto escape_for_js = [](std::string& str) {
        size_t pos = 0;
        // Escape backslashes first
        while ((pos = str.find("\\", pos)) != std::string::npos) {
            str.replace(pos, 1, "\\\\");
            pos += 2;
        }
        // Then escape double quotes (since we use double quotes in JavaScript)
        pos = 0;
        while ((pos = str.find("\"", pos)) != std::string::npos) {
            str.replace(pos, 1, "\\\"");
            pos += 2;
        }
        // Also escape single quotes to prevent syntax errors
        pos = 0;
        while ((pos = str.find("'", pos)) != std::string::npos) {
            str.replace(pos, 1, "\\'");
            pos += 2;
        }
    };
    
    escape_for_js(escaped_value);
    escape_for_js(escaped_selector);
    
    // Step 3: Advanced form interaction script for modern frameworks
    std::string enhanced_js = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector(\"" + escaped_selector + "\"); "
        "    if (!element) return 'ELEMENT_NOT_FOUND'; "
        "    "
        "    // Advanced focus and activation sequence "
        "    element.focus(); "
        "    element.click(); "
        "    "
        "    // Simulate user typing behavior "
        "    element.value = ''; "
        "    "
        "    // Dispatch pre-input events "
        "    element.dispatchEvent(new Event('focus', { bubbles: true })); "
        "    element.dispatchEvent(new Event('focusin', { bubbles: true })); "
        "    "
        "    // Set the value "
        "    element.value = \"" + escaped_value + "\"; "
        "    "
        "    // Comprehensive event dispatching for maximum compatibility "
        "    var events = ['input', 'keydown', 'keypress', 'keyup', 'change']; "
        "    for (var i = 0; i < events.length; i++) { "
        "      var event = new Event(events[i], { bubbles: true, cancelable: true }); "
        "      if (events[i].startsWith('key')) { "
        "        event.keyCode = 13; // Enter key for search forms "
        "        event.which = 13; "
        "      } "
        "      element.dispatchEvent(event); "
        "    } "
        "    "
        "    // React/Vue.js specific handling "
        "    if (element._valueTracker) { "
        "      element._valueTracker.setValue(\"\"); "
        "      element._valueTracker.setValue(\"" + escaped_value + "\"); "
        "    } "
        "    "
        "    // Angular specific handling "
        "    if (element.ng339 || element.ng294) { "
        "      var scope = angular.element(element).scope(); "
        "      if (scope) { "
        "        scope.$apply(); "
        "      } "
        "    } "
        "    "
        "    return 'ENHANCED_FILL_SUCCESS'; "
        "  } catch(e) { "
        "    return 'ENHANCED_FILL_ERROR: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(enhanced_js);
    debug_output("Enhanced fill result: " + result);
    
    // Step 4: Verification with retry mechanism
    if (result == "ENHANCED_FILL_SUCCESS") {
        // Allow time for JavaScript frameworks to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify the value was set
        std::string verify_js = "document.querySelector(\"" + escaped_selector + "\") ? document.querySelector(\"" + escaped_selector + "\").value : 'NOT_FOUND'";
        std::string actual_value = executeJavascriptSync(verify_js);
        
        debug_output("Enhanced fill verification - expected: '" + value + "', actual: '" + actual_value + "'");
        
        if (actual_value == value || actual_value == escaped_value) {
            debug_output("Enhanced fill verification: SUCCESS");
            return true;
        } else {
            debug_output("Enhanced fill verification: FAILED - trying fallback");
            
            // Fallback: Use the original fillInput method
            return fillInput(selector, value);
        }
    }
    
    debug_output("Enhanced fill failed, falling back to standard fillInput");
    return fillInput(selector, value);
}

// Generic helper for complex form interactions (like Google search)
bool Browser::interactWithDynamicForm(const std::string& input_selector, const std::string& value, 
                                     const std::string& submit_selector, int wait_timeout) {
    debug_output("Dynamic form interaction - input: " + input_selector + ", submit: " + submit_selector);
    
    // Step 1: Enhanced form filling
    if (!fillInputEnhanced(input_selector, value)) {
        debug_output("Dynamic form: Enhanced fill failed");
        return false;
    }
    
    // Step 2: Wait a moment for JavaScript to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Step 3: Submit the form
    if (!submit_selector.empty()) {
        if (!clickElement(submit_selector)) {
            debug_output("Dynamic form: Submit click failed");
            return false;
        }
        
        // Step 4: Wait for navigation or results
        if (wait_timeout > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_timeout));
        }
    }
    
    debug_output("Dynamic form interaction completed successfully");
    return true;
}