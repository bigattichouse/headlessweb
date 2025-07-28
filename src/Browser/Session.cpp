#include "Browser.h"
#include <json/json.h>
#include <iostream>
#include <set>

// External debug flag
extern bool g_debug;

// ========== Session Restoration ==========

void Browser::restoreSession(const Session& session) {
    try {
        // Set user agent first if present
        if (!session.getUserAgent().empty()) {
            setUserAgent(session.getUserAgent());
            wait(100); // Small delay for user agent to take effect
        }
        
        // Restore viewport if present
        auto viewport = session.getViewport();
        if (viewport.first > 0 && viewport.second > 0) {
            setViewport(viewport.first, viewport.second);
            // Wait for viewport change to complete using proper signals instead of arbitrary delay
            waitForJavaScriptCompletion(500); // Short timeout for viewport change
            debug_output("Restored viewport: " + std::to_string(viewport.first) + "x" + std::to_string(viewport.second));
        }
        
        // Navigate to current URL if present and not already there
        if (!session.getCurrentUrl().empty() && session.getCurrentUrl() != getCurrentUrl()) {
            debug_output("Loading URL: " + session.getCurrentUrl());
            loadUri(session.getCurrentUrl());
            
            // Wait for load using event-driven approach
            if (!waitForNavigationSignal(15000)) {
                std::cerr << "Warning: Page load timeout during session restore" << std::endl;
                return; // Don't try to restore state on failed page load
            }
            
            // Wait for page to be ready using event-driven approach
            waitForPageReadyEvent(5000);
            
            // Verify we can execute JavaScript
            std::string testResult = executeJavascriptSync("(function() { return 'test'; })()");
            if (testResult != "test") {
                std::cerr << "Warning: JavaScript execution not working properly" << std::endl;
                return;
            }
            
            debug_output("Page loaded successfully");
        }
        
        // Only restore state if page loaded successfully
        std::string readyState = executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return 'error'; } })()");
        if (readyState != "complete" && readyState != "interactive") {
            std::cerr << "Warning: Page not ready for state restoration (state: " << readyState << ")" << std::endl;
            return;
        }
        
        // Check if this is a file:// URL
        bool isFileUrl = session.getCurrentUrl().find("file://") == 0;
        
        // Restore state step by step with error handling
        debug_output("Starting state restoration...");
        
        // Cookies
        try {
            if (!session.getCookies().empty()) {
                for (const auto& cookie : session.getCookies()) {
                    setCookieSafe(cookie);
                }
                wait(500);
                debug_output("Restored " + std::to_string(session.getCookies().size()) + " cookies");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore cookies: " << e.what() << std::endl;
        }
        
        // Storage - skip for file:// URLs due to security restrictions
        if (!isFileUrl) {
            try {
                // Clear existing storage to ensure session isolation
                clearAllStorage();
                debug_output("Cleared existing storage for session isolation");
                
                if (!session.getLocalStorage().empty()) {
                    setLocalStorage(session.getLocalStorage());
                    wait(500);
                    debug_output("Restored localStorage");
                }
                
                if (!session.getSessionStorage().empty()) {
                    setSessionStorage(session.getSessionStorage());
                    wait(500);
                    debug_output("Restored sessionStorage");
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to restore storage: " << e.what() << std::endl;
            }
        } else {
            debug_output("Skipping storage restoration for file:// URL");
        }
        
        // Form state - use the new implementation
        try {
            if (!session.getFormFields().empty()) {
                debug_output("Restoring " + std::to_string(session.getFormFields().size()) + " form fields");
                for (const auto& field : session.getFormFields()) {
                    debug_output("  Restoring: " + field.selector + " = " + field.value + " (checked: " + std::to_string(field.checked) + ")");
                }
                restoreFormState(session.getFormFields());
                wait(500);
                debug_output("Restored form state");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore form state: " << e.what() << std::endl;
        }
        
        // Scroll positions - use the new implementation
        try {
            if (!session.getAllScrollPositions().empty()) {
                restoreScrollPositions(session.getAllScrollPositions());
                wait(500);
                debug_output("Restored scroll positions");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore scroll positions: " << e.what() << std::endl;
        }
        
        // Active elements
        try {
            if (!session.getActiveElements().empty()) {
                restoreActiveElements(session.getActiveElements());
                wait(200);
                debug_output("Restored active elements");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore active elements: " << e.what() << std::endl;
        }
        
        // *** NEW: Custom Attributes Restoration ***
        try {
            Json::Value customAttributesState = session.getExtractedState("customAttributes");
            if (!customAttributesState.isNull()) {
                restoreCustomAttributesFromState(customAttributesState);
                wait(500);
                debug_output("Restored custom attributes");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore custom attributes: " << e.what() << std::endl;
        }
        
        // Custom state restoration - existing mechanism
        try {
            auto extractedState = session.getAllExtractedState();
            if (!extractedState.empty()) {
                restoreCustomState(extractedState);
                wait(200);
                debug_output("Restored custom state");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore custom state: " << e.what() << std::endl;
        }
        
        // Final wait for everything to settle
        wait(1000);
        debug_output("Session restoration complete");
        
    } catch (const std::exception& e) {
        std::cerr << "Error in session restoration: " << e.what() << std::endl;
    }
}

// ========== Session State Update ==========

void Browser::updateSessionState(Session& session) {
    try {
        // Always update current URL first (this should never fail)
        session.setCurrentUrl(getCurrentUrl());
        
        // Try a simple JavaScript test first
        std::string testResult = executeJavascriptSync("(function() { try { return 'alive'; } catch(e) { return 'dead'; } })()");
        if (testResult != "alive") {
            std::cerr << "Warning: JavaScript execution not working, but preserving session URL context" << std::endl;
            // Still update last accessed time and preserve URL context
            session.updateLastAccessed();
            return;
        }
        
        // Check if we can safely execute JavaScript
        std::string readyState = executeJavascriptSync("(function() { try { return document.readyState || 'unknown'; } catch(e) { return 'error'; } })()");
        
        if (readyState == "error" || readyState.empty() || readyState == "unknown") {
            std::cerr << "Warning: Cannot determine page state, skipping detailed state extraction" << std::endl;
            session.updateLastAccessed();
            return;
        }
        
        // Check if this is a file:// URL
        bool isFileUrl = getCurrentUrl().find("file://") == 0;
        
        // Only proceed if we have a properly loaded page
        if (readyState == "complete" || readyState == "interactive") {
            // Safe state extraction with individual try-catch blocks
            try {
                session.setPageHash(extractPageHash());
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract page hash: " << e.what() << std::endl;
            }
            
            try {
                session.setDocumentReadyState(readyState);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to set document ready state: " << e.what() << std::endl;
            }
            
            // Cookies - use the new implementation
            try {
                getCookiesAsync([&session](std::vector<Cookie> cookies) {
                    if (g_debug) {
                        std::cerr << "Debug: Extracted " << cookies.size() << " cookies" << std::endl;
                        for (const auto& cookie : cookies) {
                            std::cerr << "  Cookie: " << cookie.name << " = " << cookie.value << std::endl;
                        }
                    }
                    session.setCookies(cookies);
                });
                waitForJavaScriptCompletion(1000); // Shorter timeout
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract cookies: " << e.what() << std::endl;
            }
            
            // Storage - skip for file:// URLs but use new implementations
            if (!isFileUrl) {
                try {
                    auto localStorage = getLocalStorage();
                    session.setLocalStorage(localStorage);
                    debug_output("Extracted " + std::to_string(localStorage.size()) + " localStorage items");
                } catch (const std::exception& e) {
                    // Expected for file:// URLs
                }
                
                try {
                    auto sessionStorage = getSessionStorage();
                    session.setSessionStorage(sessionStorage);
                    debug_output("Extracted " + std::to_string(sessionStorage.size()) + " sessionStorage items");
                } catch (const std::exception& e) {
                    // Expected for file:// URLs
                }
            }
            
            // Form state - use the new implementation
            try {
                auto formFields = extractFormState();
                session.setFormFields(formFields);
                debug_output("Extracted " + std::to_string(formFields.size()) + " form fields");
                for (const auto& field : formFields) {
                    debug_output("  Field: " + field.selector + " = " + field.value + " (checked: " + std::to_string(field.checked) + ")");
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract form state: " << e.what() << std::endl;
            }
            
            // Active elements - use the new implementation
            try {
                auto activeElements = extractActiveElements();
                session.setActiveElements(activeElements);
                debug_output("Extracted " + std::to_string(activeElements.size()) + " active elements");
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract active elements: " << e.what() << std::endl;
            }
            
            // Scroll positions - use the new implementation
            try {
                auto scrollPositions = extractAllScrollPositions();
                debug_output("Extracted scroll positions:");
                for (const auto& [selector, pos] : scrollPositions) {
                    session.setScrollPosition(selector, pos.first, pos.second);
                    debug_output("  " + selector + ": " + std::to_string(pos.first) + ", " + std::to_string(pos.second));
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract scroll positions: " << e.what() << std::endl;
            }
            
            // *** NEW: Custom Attributes Extraction ***
            try {
                std::string customAttributesScript = extractCustomAttributesScript();
                std::string attributesResult = executeJavascriptSync(customAttributesScript);
                
                if (!attributesResult.empty() && attributesResult != "undefined" && attributesResult != "{}") {
                    Json::Value attributesJson;
                    Json::Reader reader;
                    if (reader.parse(attributesResult, attributesJson)) {
                        session.setExtractedState("customAttributes", attributesJson);
                        debug_output("Extracted custom attributes: " + attributesResult);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract custom attributes: " << e.what() << std::endl;
            }
            
            // Custom state - existing extractors
            try {
                if (!session.getStateExtractors().empty()) {
                    auto customState = extractCustomState(session.getStateExtractors());
                    auto memberNames = customState.getMemberNames();
                    for (const auto& key : memberNames) {
                        session.setExtractedState(key, customState[key]);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract custom state: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Warning: Page not in ready state (" << readyState << "), skipping detailed extraction" << std::endl;
        }
        
        // Always update last accessed time
        session.updateLastAccessed();
        
    } catch (const std::exception& e) {
        std::cerr << "Error in updateSessionState: " << e.what() << std::endl;
        // At minimum, update the timestamp
        try {
            session.updateLastAccessed();
        } catch (...) {
            // Even this failed, just continue
        }
    }
}

// ========== Safe Session Restoration ==========

bool Browser::restoreSessionSafely(const Session& session) {
    try {
        restoreSession(session);
        return isPageLoaded();
    } catch (const std::exception& e) {
        return false;
    }
}

bool Browser::validateSession(const Session& session) const {
    return !session.getName().empty();
}

// ========== Form State Management ==========

std::vector<FormField> Browser::extractFormState() {
    std::vector<FormField> fields;
    
    // Extract input fields
    std::string inputJs = R"(
        (function() {
            const inputs = document.querySelectorAll('input, textarea, select');
            const result = [];
            
            inputs.forEach((el, index) => {
                const field = {};
                field.selector = el.id ? '#' + el.id : 
                                (el.name ? '[name="' + el.name + '"]' : 
                                ':nth-child(' + (Array.from(el.parentNode.children).indexOf(el) + 1) + ')');
                field.name = el.name || '';
                field.id = el.id || '';
                field.value = el.value || '';
                field.checked = el.type === 'checkbox' || el.type === 'radio' ? el.checked : false;
                field.type = el.type || el.tagName.toLowerCase();
                result.push(field);
            });
            
            return JSON.stringify(result);
        })()
    )";
    
    std::string result = executeJavascriptSync(inputJs);
    
    if (!result.empty() && result != "undefined") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isArray()) {
                for (const auto& item : root) {
                    FormField field;
                    field.selector = item["selector"].asString();
                    field.name = item["name"].asString();
                    field.id = item["id"].asString();
                    field.value = item["value"].asString();
                    field.checked = item["checked"].asBool();
                    field.type = item["type"].asString();
                    fields.push_back(field);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing form state: " << e.what() << std::endl;
        }
    }
    
    return fields;
}

void Browser::restoreFormState(const std::vector<FormField>& fields) {
    for (const auto& field : fields) {
        try {
            // Determine element type dynamically since tagName isn't available
            if (field.type == "checkbox" || field.type == "radio") {
                // Handle checkboxes and radio buttons
                if (field.checked) {
                    checkElement(field.selector);
                } else {
                    uncheckElement(field.selector);
                }
            } else if (field.type == "select" || field.type == "select-one" || field.type == "select-multiple") {
                // Handle select elements
                selectOption(field.selector, field.value);
            } else {
                // Check if it's a select element by querying the DOM
                std::string isSelectJs = "document.querySelector('" + field.selector + "') && "
                                        "document.querySelector('" + field.selector + "').tagName === 'SELECT'";
                std::string isSelect = executeJavascriptSync(isSelectJs);
                
                if (isSelect == "true") {
                    selectOption(field.selector, field.value);
                } else {
                    // Handle text inputs and textareas
                    fillInput(field.selector, field.value);
                }
            }
            
            // Small delay between form field restorations
            wait(50);
        } catch (const std::exception& e) {
            std::cerr << "Error restoring form field " << field.selector << ": " << e.what() << std::endl;
        }
    }
}

// ========== Active Elements Management ==========

std::set<std::string> Browser::extractActiveElements() {
    std::set<std::string> elements;
    
    std::string js = R"(
        (function() {
            const activeElements = [];
            const focusedEl = document.activeElement;
            if (focusedEl && focusedEl !== document.body) {
                if (focusedEl.id) {
                    activeElements.push('#' + focusedEl.id);
                } else if (focusedEl.name) {
                    activeElements.push('[name="' + focusedEl.name + '"]');
                }
            }
            return JSON.stringify(activeElements);
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    if (!result.empty() && result != "undefined") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isArray()) {
                for (const auto& item : root) {
                    elements.insert(item.asString());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing active elements: " << e.what() << std::endl;
        }
    }
    
    return elements;
}

void Browser::restoreActiveElements(const std::set<std::string>& elements) {
    for (const auto& selector : elements) {
        focusElement(selector);
        // Only focus one element
        break;
    }
}

// ========== Page State Extraction ==========

std::string Browser::extractPageHash() {
    return executeJavascriptSync("window.location.hash || ''");
}

std::string Browser::extractDocumentReadyState() {
    return executeJavascriptSync("document.readyState || 'unknown'");
}

// ========== Scroll Position Management ==========

std::map<std::string, std::pair<int, int>> Browser::extractAllScrollPositions() {
    std::map<std::string, std::pair<int, int>> positions;
    
    // Extract main window scroll position
    std::string mainScrollJs = R"(
        JSON.stringify({
            window: {
                x: window.pageXOffset || document.documentElement.scrollLeft || 0,
                y: window.pageYOffset || document.documentElement.scrollTop || 0
            }
        })
    )";
    
    std::string result = executeJavascriptSync(mainScrollJs);
    
    if (!result.empty() && result != "undefined") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isObject() && root.isMember("window")) {
                int x = root["window"]["x"].asInt();
                int y = root["window"]["y"].asInt();
                positions["window"] = std::make_pair(x, y);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing scroll positions: " << e.what() << std::endl;
        }
    }
    
    // TODO: Extract scroll positions for scrollable divs/elements
    
    return positions;
}

void Browser::restoreScrollPositions(const std::map<std::string, std::pair<int, int>>& positions) {
    for (const auto& [selector, pos] : positions) {
        if (selector == "window") {
            setScrollPosition(pos.first, pos.second);
        }
        // TODO: Handle other scrollable elements
    }
}

// ========== Custom State Management ==========

Json::Value Browser::extractCustomState(const std::map<std::string, std::string>& extractors) {
    Json::Value result;
    
    for (const auto& [name, script] : extractors) {
        try {
            std::string value = executeJavascriptSync(script);
            if (!value.empty() && value != "undefined") {
                // Try to parse as JSON first
                Json::Value parsed;
                Json::Reader reader;
                if (reader.parse(value, parsed)) {
                    result[name] = parsed;
                } else {
                    // Store as string if not valid JSON
                    result[name] = value;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to extract custom state '" << name << "': " << e.what() << std::endl;
        }
    }
    
    return result;
}

void Browser::restoreCustomState(const std::map<std::string, Json::Value>& state) {
    for (const auto& [name, value] : state) {
        try {
            // Convert JSON value back to string and execute as JavaScript
            std::string valueStr;
            if (value.isString()) {
                valueStr = value.asString();
            } else {
                Json::FastWriter writer;
                valueStr = writer.write(value);
            }
            
            // Store in a window variable for access
            std::string js = "window['_hweb_custom_" + name + "'] = " + valueStr + "; 'restored';";
            executeJavascriptSync(js);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore custom state '" << name << "': " << e.what() << std::endl;
        }
    }
}

// ========== NEW: Custom Attributes Management ==========

std::string Browser::extractCustomAttributesScript() {
    return R"(
        (function() {
            const elements = document.querySelectorAll('*');
            const result = {};
            
            elements.forEach((el) => {
                const customAttrs = {};
                let hasCustomAttrs = false;
                
                // Look for data-* attributes and other non-standard attributes
                for (let i = 0; i < el.attributes.length; i++) {
                    const attr = el.attributes[i];
                    const isStandardAttr = [
                        'id', 'class', 'name', 'type', 'value', 'checked', 'selected', 
                        'src', 'href', 'placeholder', 'title', 'alt', 'for', 'action',
                        'method', 'target', 'rel', 'style', 'tabindex', 'role'
                    ].includes(attr.name);
                    
                    if (attr.name.startsWith('data-') || !isStandardAttr) {
                        customAttrs[attr.name] = attr.value;
                        hasCustomAttrs = true;
                    }
                }
                
                if (hasCustomAttrs) {
                    // Create a reliable selector
                    let selector = '';
                    if (el.id) {
                        selector = '#' + el.id;
                    } else if (el.name) {
                        selector = '[name="' + el.name + '"]';
                    } else {
                        // Use tag + nth-child as fallback
                        const parent = el.parentNode;
                        if (parent) {
                            const index = Array.from(parent.children).indexOf(el) + 1;
                            selector = el.tagName.toLowerCase() + ':nth-child(' + index + ')';
                        } else {
                            selector = el.tagName.toLowerCase();
                        }
                    }
                    
                    result[selector] = customAttrs;
                }
            });
            
            return JSON.stringify(result);
        })()
    )";
}

void Browser::restoreCustomAttributesFromState(const Json::Value& attributesState) {
    if (!attributesState.isObject()) {
        return;
    }
    
    for (const auto& selector : attributesState.getMemberNames()) {
        const Json::Value& attributes = attributesState[selector];
        if (!attributes.isObject()) {
            continue;
        }
        
        try {
            for (const auto& attrName : attributes.getMemberNames()) {
                std::string attrValue = attributes[attrName].asString();
                
                // Use the existing setAttribute method which we know works
                if (setAttribute(selector, attrName, attrValue)) {
                    debug_output("Restored attribute: " + selector + "[" + attrName + "] = " + attrValue);
                } else {
                    debug_output("Failed to restore attribute: " + selector + "[" + attrName + "]");
                }
                
                wait(50); // Small delay between attribute restorations
            }
        } catch (const std::exception& e) {
            std::cerr << "Error restoring attributes for " << selector << ": " << e.what() << std::endl;
        }
    }
}
