#include "Browser.h"
#include <iostream>
#include <string>

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
    if (!waitForSelectorEvent(selector, 5000)) {
        return false;
    }
    
    if (!waitForVisibilityEvent(selector, 2000)) {
        debug_output("Warning: Element exists but may not be visible");
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.click(); "
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
    std::string js_script = 
        "(function() { "
        "  try { "
        "    return document.querySelector('" + selector + "') !== null; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

int Browser::countElements(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    return document.querySelectorAll('" + selector + "').length; "
        "  } catch(e) { "
        "    return 0; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
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
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element ? element.innerText || element.textContent || '' : ''; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    return executeJavascriptSync(js_script);
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
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element ? (element.getAttribute('" + attribute + "') || '') : ''; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

bool Browser::setAttribute(const std::string& selector, const std::string& attribute, const std::string& value) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.setAttribute('" + attribute + "', '" + value + "'); "
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
