#include "Browser.h"
#include "../Debug.h"
#include <iostream>
#include <algorithm>

// External debug flag
extern bool g_debug;

// ========== Advanced Waiting Method Implementations ==========

bool Browser::waitForTextAdvanced(const std::string& text, int timeout_ms, bool case_sensitive, bool exact_match) {
    debug_output("Waiting for text (advanced): " + text + 
                " (case_sensitive=" + std::to_string(case_sensitive) + 
                ", exact_match=" + std::to_string(exact_match) + ")");
    
    // Escape the text for JavaScript
    std::string escaped_text = text;
    size_t pos = 0;
    while ((pos = escaped_text.find("'", pos)) != std::string::npos) {
        escaped_text.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string condition;
    if (exact_match) {
        if (case_sensitive) {
            condition = "document.body && document.body.innerText.trim() === '" + escaped_text + "'";
        } else {
            condition = "document.body && document.body.innerText.trim().toLowerCase() === '" + escaped_text + "'.toLowerCase()";
        }
    } else {
        if (case_sensitive) {
            condition = "document.body && document.body.innerText.includes('" + escaped_text + "')";
        } else {
            condition = "document.body && document.body.innerText.toLowerCase().includes('" + escaped_text + "'.toLowerCase())";
        }
    }
    
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForNetworkIdle(int idle_time_ms, int timeout_ms) {
    debug_output("Waiting for network idle: " + std::to_string(idle_time_ms) + "ms idle time");
    
    // JavaScript to monitor network activity
    std::string networkScript = R"(
        (function(idleTime, totalTimeout) {
            window._hweb_event_result = undefined;
            window._hweb_network_requests = window._hweb_network_requests || 0;
            window._hweb_last_activity = Date.now();
            
            // Override XMLHttpRequest to track requests
            if (!window._hweb_xhr_overridden) {
                const originalXHR = XMLHttpRequest.prototype.open;
                XMLHttpRequest.prototype.open = function() {
                    window._hweb_network_requests++;
                    window._hweb_last_activity = Date.now();
                    
                    this.addEventListener('loadend', function() {
                        window._hweb_network_requests = Math.max(0, window._hweb_network_requests - 1);
                        window._hweb_last_activity = Date.now();
                    });
                    
                    return originalXHR.apply(this, arguments);
                };
                
                // Override fetch too
                const originalFetch = window.fetch;
                window.fetch = function() {
                    window._hweb_network_requests++;
                    window._hweb_last_activity = Date.now();
                    
                    return originalFetch.apply(this, arguments).finally(() => {
                        window._hweb_network_requests = Math.max(0, window._hweb_network_requests - 1);
                        window._hweb_last_activity = Date.now();
                    });
                };
                
                window._hweb_xhr_overridden = true;
            }
            
            // Check for idle state
            const checkIdle = () => {
                const now = Date.now();
                const timeSinceActivity = now - window._hweb_last_activity;
                
                if (window._hweb_network_requests === 0 && timeSinceActivity >= idleTime) {
                    window._hweb_event_result = true;
                } else if (now - window._hweb_start_time >= totalTimeout) {
                    window._hweb_event_result = false;
                } else {
                    setTimeout(checkIdle, 100);
                }
            };
            
            window._hweb_start_time = Date.now();
            setTimeout(checkIdle, idleTime);
            
        })()" + std::to_string(idle_time_ms) + ", " + std::to_string(timeout_ms) + R"();
    )";
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascriptSync(networkScript);
    
    int elapsed = 0;
    const int check_interval = 200;
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            debug_output("Network idle detected");
            return true;
        } else if (result == "false") {
            debug_output("Network idle timeout");
            return false;
        }
    }
    
    debug_output("Network idle timeout");
    return false;
}

bool Browser::waitForNetworkRequest(const std::string& url_pattern, int timeout_ms) {
    debug_output("Waiting for network request matching: " + url_pattern);
    
    // JavaScript to monitor for specific network requests
    std::string escaped_pattern = url_pattern;
    size_t pos = 0;
    while ((pos = escaped_pattern.find("'", pos)) != std::string::npos) {
        escaped_pattern.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string networkScript = R"(
        (function(pattern, totalTimeout) {
            window._hweb_event_result = undefined;
            
            if (!window._hweb_request_monitor) {
                const originalXHR = XMLHttpRequest.prototype.open;
                XMLHttpRequest.prototype.open = function(method, url) {
                    try {
                        if (url.includes(pattern) || url.match(new RegExp(pattern))) {
                            window._hweb_event_result = true;
                        }
                    } catch(e) {
                        if (url.includes(pattern)) {
                            window._hweb_event_result = true;
                        }
                    }
                    return originalXHR.apply(this, arguments);
                };
                
                const originalFetch = window.fetch;
                window.fetch = function(url) {
                    try {
                        if (url.includes && (url.includes(pattern) || url.match(new RegExp(pattern)))) {
                            window._hweb_event_result = true;
                        }
                    } catch(e) {
                        if (url.includes && url.includes(pattern)) {
                            window._hweb_event_result = true;
                        }
                    }
                    return originalFetch.apply(this, arguments);
                };
                
                window._hweb_request_monitor = true;
            }
            
            setTimeout(() => {
                if (window._hweb_event_result === undefined) {
                    window._hweb_event_result = false;
                }
            }, totalTimeout);
            
        })(')" + escaped_pattern + "', " + std::to_string(timeout_ms) + R"();
    )";
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascriptSync(networkScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            debug_output("Network request detected: " + url_pattern);
            return true;
        } else if (result == "false") {
            debug_output("Network request timeout: " + url_pattern);
            return false;
        }
    }
    
    debug_output("Network request timeout: " + url_pattern);
    return false;
}

bool Browser::waitForElementVisible(const std::string& selector, int timeout_ms) {
    debug_output("Waiting for element visible: " + selector);
    return waitForVisibilityEvent(selector, timeout_ms);
}

bool Browser::waitForElementCount(const std::string& selector, const std::string& operator_str, int expected_count, int timeout_ms) {
    debug_output("Waiting for element count: " + selector + " " + operator_str + " " + std::to_string(expected_count));
    
    std::string escaped_selector = selector;
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string condition = "(function() { "
        "var count = document.querySelectorAll('" + escaped_selector + "').length; ";
    
    if (operator_str == ">") {
        condition += "return count > " + std::to_string(expected_count) + ";";
    } else if (operator_str == "<") {
        condition += "return count < " + std::to_string(expected_count) + ";";
    } else if (operator_str == ">=") {
        condition += "return count >= " + std::to_string(expected_count) + ";";
    } else if (operator_str == "<=") {
        condition += "return count <= " + std::to_string(expected_count) + ";";
    } else if (operator_str == "!=") {
        condition += "return count != " + std::to_string(expected_count) + ";";
    } else { // default to ==
        condition += "return count == " + std::to_string(expected_count) + ";";
    }
    
    condition += " })()";
    
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForAttribute(const std::string& selector, const std::string& attribute, const std::string& expected_value, int timeout_ms) {
    debug_output("Waiting for attribute: " + selector + "[" + attribute + "='" + expected_value + "']");
    
    std::string escaped_selector = selector;
    std::string escaped_attribute = attribute;
    std::string escaped_value = expected_value;
    
    // Escape strings for JavaScript
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped_attribute.find("'", pos)) != std::string::npos) {
        escaped_attribute.replace(pos, 1, "\\'");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string condition = "(function() { "
        "var el = document.querySelector('" + escaped_selector + "'); "
        "if (!el) return false; "
        "var attr = el.getAttribute('" + escaped_attribute + "'); "
        "return attr === '" + escaped_value + "'; "
        "})()";
    
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForUrlChange(const std::string& pattern, int timeout_ms) {
    debug_output("Waiting for URL change matching: " + pattern);
    
    std::string initial_url = getCurrentUrl();
    
    std::string escaped_pattern = pattern;
    size_t pos = 0;
    while ((pos = escaped_pattern.find("'", pos)) != std::string::npos) {
        escaped_pattern.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string condition = "(function() { "
        "var currentUrl = window.location.href; "
        "if (currentUrl !== '" + initial_url + "') { "
        "  if ('" + pattern + "' === '' || currentUrl.includes('" + escaped_pattern + "')) { "
        "    return true; "
        "  } "
        "  try { "
        "    if (currentUrl.match(new RegExp('" + escaped_pattern + "'))) return true; "
        "  } catch(e) {} "
        "} "
        "return false; "
        "})()";
    
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForTitleChange(const std::string& pattern, int timeout_ms) {
    debug_output("Waiting for title change matching: " + pattern);
    
    std::string initial_title = getPageTitle();
    
    std::string escaped_pattern = pattern;
    std::string escaped_initial = initial_title;
    size_t pos = 0;
    while ((pos = escaped_pattern.find("'", pos)) != std::string::npos) {
        escaped_pattern.replace(pos, 1, "\\'");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped_initial.find("'", pos)) != std::string::npos) {
        escaped_initial.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string condition = "(function() { "
        "var currentTitle = document.title; "
        "if (currentTitle !== '" + escaped_initial + "') { "
        "  if ('" + pattern + "' === '' || currentTitle.includes('" + escaped_pattern + "')) { "
        "    return true; "
        "  } "
        "  try { "
        "    if (currentTitle.match(new RegExp('" + escaped_pattern + "'))) return true; "
        "  } catch(e) {} "
        "} "
        "return false; "
        "})()";
    
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForSPANavigation(const std::string& route, int timeout_ms) {
    debug_output("Waiting for SPA navigation to: " + (route.empty() ? "any route" : route));
    
    std::string initial_url = getCurrentUrl();
    
    std::string escaped_route = route;
    std::string escaped_initial = initial_url;
    size_t pos = 0;
    while ((pos = escaped_route.find("'", pos)) != std::string::npos) {
        escaped_route.replace(pos, 1, "\\'");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped_initial.find("'", pos)) != std::string::npos) {
        escaped_initial.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string condition;
    if (route.empty()) {
        // Wait for any URL change (including hash changes)
        condition = "(function() { "
            "return window.location.href !== '" + escaped_initial + "' || "
            "       window.location.hash !== '" + getCurrentUrl() + "'.split('#')[1] || ''; "
            "})()";
    } else {
        // Wait for specific route
        condition = "(function() { "
            "var currentPath = window.location.pathname + window.location.hash; "
            "return currentPath.includes('" + escaped_route + "') || "
            "       (function() { try { return currentPath.match(new RegExp('" + escaped_route + "')); } catch(e) { return false; } })(); "
            "})()";
    }
    
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForFrameworkReady(const std::string& framework, int timeout_ms) {
    debug_output("Waiting for framework ready: " + framework);
    
    std::string condition;
    
    if (framework == "auto" || framework.empty()) {
        // Auto-detect common frameworks
        condition = "(function() { "
            "return (typeof jQuery !== 'undefined' && jQuery.isReady) || "
            "       (typeof angular !== 'undefined' && angular.element(document).injector()) || "
            "       (typeof React !== 'undefined') || "
            "       (typeof Vue !== 'undefined') || "
            "       (window.APP_READY === true) || "
            "       document.readyState === 'complete'; "
            "})()";
    } else if (framework == "jquery") {
        condition = "typeof jQuery !== 'undefined' && jQuery.isReady";
    } else if (framework == "angular") {
        condition = "typeof angular !== 'undefined' && angular.element(document).injector()";
    } else if (framework == "react") {
        condition = "typeof React !== 'undefined'";
    } else if (framework == "vue") {
        condition = "typeof Vue !== 'undefined'";
    } else {
        // Custom framework check
        std::string escaped_framework = framework;
        size_t pos = 0;
        while ((pos = escaped_framework.find("'", pos)) != std::string::npos) {
            escaped_framework.replace(pos, 1, "\\'");
            pos += 2;
        }
        condition = "typeof " + escaped_framework + " !== 'undefined'";
    }
    
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForDOMChange(const std::string& selector, int timeout_ms) {
    debug_output("Waiting for DOM change on: " + selector);
    
    std::string escaped_selector = selector;
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string observerScript = R"(
        (function(selector, timeout) {
            window._hweb_event_result = undefined;
            
            const targetNode = selector ? document.querySelector(selector) : document.documentElement;
            if (!targetNode) {
                window._hweb_event_result = false;
                return;
            }
            
            const observer = new MutationObserver((mutations) => {
                if (mutations.length > 0) {
                    observer.disconnect();
                    window._hweb_event_result = true;
                }
            });
            
            observer.observe(targetNode, {
                childList: true,
                subtree: true,
                attributes: true,
                characterData: true
            });
            
            setTimeout(() => {
                observer.disconnect();
                if (window._hweb_event_result === undefined) {
                    window._hweb_event_result = false;
                }
            }, timeout);
            
        })(')" + escaped_selector + "', " + std::to_string(timeout_ms) + R"();
    )";
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascriptSync(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            debug_output("DOM change detected");
            return true;
        } else if (result == "false") {
            debug_output("DOM change timeout");
            return false;
        }
    }
    
    debug_output("DOM change timeout");
    return false;
}

bool Browser::waitForContentChange(const std::string& selector, const std::string& property, int timeout_ms) {
    debug_output("Waiting for content change: " + selector + "." + property);
    
    std::string escaped_selector = selector;
    std::string escaped_property = property;
    
    size_t pos = 0;
    while ((pos = escaped_selector.find("'", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\'");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped_property.find("'", pos)) != std::string::npos) {
        escaped_property.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string observerScript = R"(
        (function(selector, property, timeout) {
            window._hweb_event_result = undefined;
            
            const element = document.querySelector(selector);
            if (!element) {
                window._hweb_event_result = false;
                return;
            }
            
            let initialValue;
            if (property === 'text' || property === 'innerText') {
                initialValue = element.innerText || element.textContent;
            } else if (property === 'html' || property === 'innerHTML') {
                initialValue = element.innerHTML;
            } else if (property === 'value') {
                initialValue = element.value;
            } else {
                initialValue = element[property];
            }
            
            const checkChange = () => {
                let currentValue;
                if (property === 'text' || property === 'innerText') {
                    currentValue = element.innerText || element.textContent;
                } else if (property === 'html' || property === 'innerHTML') {
                    currentValue = element.innerHTML;
                } else if (property === 'value') {
                    currentValue = element.value;
                } else {
                    currentValue = element[property];
                }
                
                if (currentValue !== initialValue) {
                    window._hweb_event_result = true;
                } else {
                    setTimeout(checkChange, 100);
                }
            };
            
            setTimeout(() => {
                if (window._hweb_event_result === undefined) {
                    window._hweb_event_result = false;
                }
            }, timeout);
            
            // Start checking after a small delay
            setTimeout(checkChange, 100);
            
        })(')" + escaped_selector + "', '" + escaped_property + "', " + std::to_string(timeout_ms) + R"();
    )";
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascriptSync(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            debug_output("Content change detected");
            return true;
        } else if (result == "false") {
            debug_output("Content change timeout");
            return false;
        }
    }
    
    debug_output("Content change timeout");
    return false;
}
