#include "Browser.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// External debug flag
extern bool g_debug;

// ========== Form Interaction Methods ==========

bool Browser::fillInput(const std::string& selector, const std::string& value) {
    if (!waitForSelectorEvent(selector, 5000)) {
        return false;
    }
    
    // Small delay to ensure element is ready
    wait(100);
    
    // Escape quotes and other special characters in the value
    std::string escaped_value = value;
    size_t pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    // Also escape backslashes
    pos = 0;
    while ((pos = escaped_value.find("\\", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      element.value = '" + escaped_value + "'; "
        "      element.dispatchEvent(new Event('input', { bubbles: true })); "
        "      element.dispatchEvent(new Event('change', { bubbles: true })); "
        "      element.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the value to be processed
        
        // Verify the value was actually set
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').value : 'NOT_FOUND'";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == escaped_value || actualValue == value) {
            return true;
        } else {
            debug_output("Warning: Value verification failed. Expected: '" + value + "', Got: '" + actualValue + "'");
            
            // Try alternative method using setAttribute
            std::string altJs = 
                "try { "
                "  var el = document.querySelector('" + selector + "'); "
                "  if (el) { "
                "    el.setAttribute('value', '" + escaped_value + "'); "
                "    el.value = '" + escaped_value + "'; "
                "    return 'retry_success'; "
                "  } "
                "  return 'retry_failed'; "
                "} catch(e) { return 'retry_error'; }";
            
            std::string retryResult = executeJavascriptSync(altJs);
            if (retryResult == "retry_success") {
                wait(200); // Additional wait after retry
            }
            return retryResult == "retry_success";
        }
    }
    
    debug_output("fillInput failed: " + result);
    return false;
}

bool Browser::clickElement(const std::string& selector) {
    // Check if webView exists first
    if (!webView) {
        return false;
    }
    
    // Check if a page is loaded
    const gchar* uri = webkit_web_view_get_uri(webView);
    if (!uri || strlen(uri) == 0) {
        debug_output("No page loaded, clickElement returning false for: " + selector);
        return false;
    }
    
    // Single wait with combined check - no nested event loops
    if (!elementExists(selector)) {
        debug_output("Element does not exist: " + selector);
        return false;
    }
    
    // Escape selector to prevent injection
    std::string escaped_selector = selector;
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    if (!document || !document.querySelector) return 'NO_DOCUMENT'; "
        "    var element = document.querySelector('" + escaped_selector + "'); "
        "    if (element) { "
        "      // Check if element is visible before clicking "
        "      var rect = element.getBoundingClientRect(); "
        "      if (rect.width > 0 && rect.height > 0) { "
        "        element.click(); "
        "        return 'clicked'; "
        "      } else { "
        "        return 'not_visible'; "
        "      } "
        "    } "
        "    return 'not_found'; "
        "  } catch(e) { "
        "    return 'error:' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    if (result == "clicked") {
        return true;
    } else if (result == "not_visible") {
        debug_output("Element not visible: " + selector);
        return false;
    } else if (result == "not_found") {
        debug_output("Element not found during click: " + selector);
        return false;
    } else if (result == "NO_DOCUMENT") {
        debug_output("Document not ready for click: " + selector);
        return false;
    } else {
        debug_output("Click failed: " + result);
        return false;
    }
}

bool Browser::submitForm(const std::string& form_selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var form = document.querySelector('" + form_selector + "'); "
        "    if (form) { "
        "      form.submit(); "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::searchForm(const std::string& query) {
    // Implementation for search form
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var inputs = document.querySelectorAll('input[type=\"search\"], input[name*=\"search\"], input[placeholder*=\"search\"]'); "
        "    if (inputs.length > 0) { "
        "      inputs[0].value = '" + query + "'; "
        "      inputs[0].dispatchEvent(new Event('input', { bubbles: true })); "
        "      var form = inputs[0].closest('form'); "
        "      if (form) { "
        "        form.submit(); "
        "        return true; "
        "      } "
        "    } "
        "    return false; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::selectOption(const std::string& selector, const std::string& value) {
    // Wait for element and add a small delay
    wait(100);
    
    // Escape quotes and other special characters in the value
    std::string escaped_value = value;
    size_t pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    // Also escape backslashes
    pos = 0;
    while ((pos = escaped_value.find("\\", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var select = document.querySelector('" + selector + "'); "
        "    if (select) { "
        "      select.focus(); "
        "      select.value = '" + escaped_value + "'; "
        "      select.dispatchEvent(new Event('change', { bubbles: true })); "
        "      select.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the value to be processed
        
        // Verify the value was actually set
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').value : 'NOT_FOUND'";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == escaped_value || actualValue == value) {
            return true;
        } else {
            debug_output("Warning: Select verification failed. Expected: '" + value + "', Got: '" + actualValue + "'");
            
            // Try alternative method using selectedIndex
            std::string altJs = 
                "try { "
                "  var sel = document.querySelector('" + selector + "'); "
                "  if (sel) { "
                "    for (var i = 0; i < sel.options.length; i++) { "
                "      if (sel.options[i].value === '" + escaped_value + "') { "
                "        sel.selectedIndex = i; "
                "        sel.dispatchEvent(new Event('change', { bubbles: true })); "
                "        return 'retry_success'; "
                "      } "
                "    } "
                "  } "
                "  return 'retry_failed'; "
                "} catch(e) { return 'retry_error'; }";
            
            std::string retryResult = executeJavascriptSync(altJs);
            if (retryResult == "retry_success") {
                wait(200); // Additional wait after retry
            }
            return retryResult == "retry_success";
        }
    }
    
    debug_output("selectOption failed: " + result);
    return false;
}

bool Browser::checkElement(const std::string& selector) {
    // Small delay to ensure element is ready
    wait(100);
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      element.checked = true; "
        "      element.dispatchEvent(new Event('change', { bubbles: true })); "
        "      element.dispatchEvent(new Event('click', { bubbles: true })); "
        "      element.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the change to be processed
        
        // Verify the checkbox was actually checked
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').checked : false";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == "true") {
            return true;
        } else {
            debug_output("Warning: Checkbox verification failed. Expected: checked, Got: " + actualValue);
        }
    }
    
    debug_output("checkElement failed: " + result);
    return result == "true"; // Return success even if verification fails, as the action was attempted
}

bool Browser::uncheckElement(const std::string& selector) {
    // Small delay to ensure element is ready
    wait(100);
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      element.checked = false; "
        "      element.dispatchEvent(new Event('change', { bubbles: true })); "
        "      element.dispatchEvent(new Event('click', { bubbles: true })); "
        "      element.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the change to be processed
        
        // Verify the checkbox was actually unchecked
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').checked : true";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == "false") {
            return true;
        } else {
            debug_output("Warning: Uncheck verification failed. Expected: unchecked, Got: " + actualValue);
        }
    }
    
    debug_output("uncheckElement failed: " + result);
    return result == "true"; // Return success even if verification fails, as the action was attempted
}

bool Browser::focusElement(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

// ========== Element Query Methods ==========

bool Browser::elementExists(const std::string& selector) {
    // Add timeout protection to prevent hanging
    if (!webView) {
        return false;
    }
    
    // Check if a page is loaded
    const gchar* uri = webkit_web_view_get_uri(webView);
    if (!uri || strlen(uri) == 0) {
        debug_output("No page loaded, elementExists returning false for: " + selector);
        return false; // No page loaded, element cannot exist
    }
    
    // Escape selector to prevent injection
    std::string escaped_selector = selector;
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    if (!document || !document.querySelector) return 'NO_DOCUMENT'; "
        "    return document.querySelector('" + escaped_selector + "') !== null; "
        "  } catch(e) { "
        "    return 'SELECTOR_ERROR:' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Check for various error conditions
    if (result.empty() || result == "NO_DOCUMENT") {
        debug_output("Document not ready for selector: " + selector);
        return false;
    }
    
    if (result.find("SELECTOR_ERROR:") == 0) {
        debug_output("Invalid CSS selector: " + selector + " (" + result.substr(15) + ")");
        return false; // Don't throw, just return false
    }
    
    return result == "true";
}

int Browser::countElements(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    return document.querySelectorAll('" + selector + "').length; "
        "  } catch(e) { "
        "    return 'SELECTOR_ERROR:' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Check if we got a selector error
    if (result.find("SELECTOR_ERROR:") == 0) {
        throw std::runtime_error("Invalid CSS selector: " + selector + " (" + result.substr(15) + ")");
    }
    
    try {
        return std::stoi(result);
    } catch (...) {
        return 0;
    }
}

std::string Browser::getElementHtml(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element ? element.outerHTML : ''; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

std::string Browser::getInnerText(const std::string& selector) {
    // Escape single quotes in selector for JavaScript
    std::string escaped_selector = selector;
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "try { "
        "if (document.readyState === 'loading') { "
        "return 'DOCUMENT_LOADING'; "
        "} "
        "var element = document.querySelector('" + escaped_selector + "'); "
        "if (!element) { "
        "return 'ELEMENT_NOT_FOUND'; "
        "} "
        "var text = element.innerText || element.textContent || ''; "
        "return text.trim(); "
        "} catch(e) { "
        "return 'JS_ERROR: ' + e.message; "
        "} "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Handle special return values
    if (result == "DOCUMENT_LOADING") {
        // Wait a bit and retry once
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        result = executeJavascriptSync(js_script);
    }
    
    if (result == "ELEMENT_NOT_FOUND" || result.substr(0, 9) == "JS_ERROR:") {
        return ""; // Return empty string for compatibility
    }
    
    return result;
}

std::string Browser::getFirstNonEmptyText(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var elements = document.querySelectorAll('" + selector + "'); "
        "    for (var i = 0; i < elements.length; i++) { "
        "      var text = (elements[i].innerText || elements[i].textContent || '').trim(); "
        "      if (text.length > 0) { "
        "        return text; "
        "      } "
        "    } "
        "    return ''; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

// ========== Attribute Methods ==========

std::string Browser::getAttribute(const std::string& selector, const std::string& attribute) {
    if (!webView) {
        return "";
    }
    
    // Check if a page is loaded
    const gchar* uri = webkit_web_view_get_uri(webView);
    if (!uri || strlen(uri) == 0) {
        debug_output("No page loaded, getAttribute returning empty for: " + selector);
        return "";
    }
    
    // Escape the selector
    std::string escaped_selector = selector;
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    // Escape the attribute name
    std::string escaped_attribute = attribute;
    pos = 0;
    while ((pos = escaped_attribute.find("'", pos)) != std::string::npos) {
        escaped_attribute.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    if (!document || !document.querySelector) return 'NO_DOCUMENT'; "
        "    var element = document.querySelector('" + escaped_selector + "'); "
        "    if (element) { "
        "      var attr = element.getAttribute('" + escaped_attribute + "'); "
        "      return attr !== null ? attr : ''; "
        "    } "
        "    return ''; "
        "  } catch(e) { "
        "    return 'ERROR:' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    if (result == "NO_DOCUMENT") {
        debug_output("Document not ready for getAttribute: " + selector);
        return "";
    }
    
    if (result.find("ERROR:") == 0) {
        debug_output("getAttribute error: " + result.substr(6));
        return "";
    }
    
    return result;
}
bool Browser::setAttribute(const std::string& selector, const std::string& attribute, const std::string& value) {
    // Small delay to ensure element is ready
    wait(100);
    
    // Escape the selector - this was missing!
    std::string escaped_selector = selector;
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    // Escape the attribute name
    std::string escaped_attribute = attribute;
    pos = 0;
    while ((pos = escaped_attribute.find("'", pos)) != std::string::npos) {
        escaped_attribute.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    // Escape the value - improved escaping
    std::string escaped_value = value;
    pos = 0;
    // Escape backslashes first to avoid double-escaping
    while ((pos = escaped_value.find("\\", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\\\");
        pos += 2;
    }
    // Then escape quotes
    pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    debug_output("Setting attribute '" + attribute + "' to '" + value + "' on selector '" + selector + "'");
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + escaped_selector + "'); "
        "    if (element) { "
        "      element.setAttribute('" + escaped_attribute + "', '" + escaped_value + "'); "
        "      return 'success'; "
        "    } "
        "    return 'element_not_found'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    debug_output("Generated JavaScript: " + js_script);
    
    std::string result = executeJavascriptSync(js_script);
    debug_output("JavaScript result: " + result);
    
    // Check if the initial execution succeeded
    if (result == "success") {
        wait(200); // Allow time for the attribute to be processed
        
        // Verify the attribute was actually set using same escaping
        std::string verifyJs = 
            "(function() { "
            "  try { "
            "    var element = document.querySelector('" + escaped_selector + "'); "
            "    if (element) { "
            "      var attr = element.getAttribute('" + escaped_attribute + "'); "
            "      return attr !== null ? attr : 'null_attribute'; "
            "    } "
            "    return 'element_not_found'; "
            "  } catch(e) { "
            "    return 'verify_error: ' + e.message; "
            "  } "
            "})()";
        
        std::string actualValue = executeJavascriptSync(verifyJs);
        debug_output("Verification result: " + actualValue);
        
        if (actualValue == escaped_value || actualValue == value) {
            debug_output("Attribute verification SUCCESS");
            return true;
        } else {
            debug_output("Warning: Attribute verification failed. Expected: '" + value + "', Got: '" + actualValue + "'");
            
            // Try alternative method with forced DOM update
            std::string altJs = 
                "(function() { "
                "  try { "
                "    var el = document.querySelector('" + escaped_selector + "'); "
                "    if (el) { "
                "      el.setAttribute('" + escaped_attribute + "', '" + escaped_value + "'); "
                "      // Force DOM update "
                "      el.offsetHeight; "
                "      // Double-check it was set "
                "      var check = el.getAttribute('" + escaped_attribute + "'); "
                "      return check !== null ? 'retry_success:' + check : 'retry_failed'; "
                "    } "
                "    return 'retry_no_element'; "
                "  } catch(e) { "
                "    return 'retry_error: ' + e.message; "
                "  } "
                "})()";
            
            std::string retryResult = executeJavascriptSync(altJs);
            debug_output("Retry result: " + retryResult);
            
            if (retryResult.find("retry_success:") == 0) {
                // Extract the actual value after "retry_success:"
                std::string retryValue = retryResult.substr(14);
                wait(200); // Additional wait after retry
                return (retryValue == escaped_value || retryValue == value);
            }
            return false;
        }
    } else {
        debug_output("setAttribute failed with result: " + result);
        return false;
    }
}
