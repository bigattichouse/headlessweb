#include "Browser.h"
#include "../Debug.h"
#include "BrowserEventBus.h"
#include "MutationTracker.h"
#include "NetworkEventTracker.h"
#include <iostream>
#include <algorithm>
#include <future>
#include <chrono>
#include <thread>

// External debug flag
extern bool g_debug;

// ========== Advanced Waiting Method Implementations ==========

bool Browser::waitForTextAdvanced(const std::string& text, int timeout_ms, bool case_sensitive, bool exact_match) {
    debug_output("Waiting for text (advanced): " + text + 
                " (case_sensitive=" + std::to_string(case_sensitive) + 
                ", exact_match=" + std::to_string(exact_match) + ")");
    
    // Enhanced text escaping for JavaScript
    std::string escaped_text = text;
    size_t pos = 0;
    // Escape single quotes
    while ((pos = escaped_text.find("'", pos)) != std::string::npos) {
        escaped_text.replace(pos, 1, "\\'");
        pos += 2;
    }
    // Escape double quotes
    pos = 0;
    while ((pos = escaped_text.find("\"", pos)) != std::string::npos) {
        escaped_text.replace(pos, 1, "\\\"");
        pos += 2;
    }
    // Escape backslashes
    pos = 0;
    while ((pos = escaped_text.find("\\", pos)) != std::string::npos && escaped_text.substr(pos, 2) != "\\'") {
        escaped_text.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    // Proper text matching with exact match requiring complete phrase boundaries
    std::string condition;
    if (exact_match) {
        // Exact match: search text must match complete text content of at least one element
        // This means "Exact" should NOT match any element containing "Exact Match Text"
        if (case_sensitive) {
            condition = "(function() { "
                       "if (!document.body) return false; "
                       "var searchText = '" + escaped_text + "'; "
                       "var elements = document.querySelectorAll('*'); "
                       "for (var i = 0; i < elements.length; i++) { "
                       "  var el = elements[i]; "
                       "  if (el.children.length === 0) { "  // Only check leaf elements (no child elements)
                       "    var text = (el.innerText || el.textContent || '').trim(); "
                       "    if (text === searchText) { "
                       "      return true; "
                       "    } "
                       "  } "
                       "} "
                       "return false; "
                       "})()";
        } else {
            condition = "(function() { "
                       "if (!document.body) return false; "
                       "var searchText = '" + escaped_text + "'.toLowerCase(); "
                       "var elements = document.querySelectorAll('*'); "
                       "for (var i = 0; i < elements.length; i++) { "
                       "  var el = elements[i]; "
                       "  if (el.children.length === 0) { "  // Only check leaf elements (no child elements)
                       "    var text = (el.innerText || el.textContent || '').trim().toLowerCase(); "
                       "    if (text === searchText) { "
                       "      return true; "
                       "    } "
                       "  } "
                       "} "
                       "return false; "
                       "})()";
        }
    } else {
        // Non-exact match: simple substring search
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
    
    // Enhanced JavaScript to monitor network activity with better reliability
    std::string networkScript = R"(
        (function(idleTime, totalTimeout) {
            window._hweb_event_result = undefined;
            
            // Initialize network tracking variables
            if (typeof window._hweb_network_requests === 'undefined') {
                window._hweb_network_requests = 0;
            }
            window._hweb_last_activity = Date.now();
            window._hweb_start_time = Date.now();
            
            // Override XMLHttpRequest to track requests with enhanced error handling
            if (!window._hweb_xhr_overridden) {
                try {
                    const originalOpen = XMLHttpRequest.prototype.open;
                    const originalSend = XMLHttpRequest.prototype.send;
                    
                    // Override open to set up event listeners
                    XMLHttpRequest.prototype.open = function() {
                        try {
                            // Add event listeners for request completion
                            this.addEventListener('loadend', function() {
                                try {
                                    window._hweb_network_requests = Math.max(0, (window._hweb_network_requests || 1) - 1);
                                    window._hweb_last_activity = Date.now();
                                } catch(e) {}
                            });
                            
                            this.addEventListener('error', function() {
                                try {
                                    window._hweb_network_requests = Math.max(0, (window._hweb_network_requests || 1) - 1);
                                    window._hweb_last_activity = Date.now();
                                } catch(e) {}
                            });
                            
                            this.addEventListener('abort', function() {
                                try {
                                    window._hweb_network_requests = Math.max(0, (window._hweb_network_requests || 1) - 1);
                                    window._hweb_last_activity = Date.now();
                                } catch(e) {}
                            });
                            
                        } catch(e) {}
                        return originalOpen.apply(this, arguments);
                    };
                    
                    // Override send to actually count the request
                    XMLHttpRequest.prototype.send = function() {
                        try {
                            window._hweb_network_requests = (window._hweb_network_requests || 0) + 1;
                            window._hweb_last_activity = Date.now();
                        } catch(e) {}
                        return originalSend.apply(this, arguments);
                    };
                    
                    // Override fetch with enhanced error handling
                    if (window.fetch) {
                        const originalFetch = window.fetch;
                        window.fetch = function() {
                            try {
                                window._hweb_network_requests = (window._hweb_network_requests || 0) + 1;
                                window._hweb_last_activity = Date.now();
                                
                                return originalFetch.apply(this, arguments).finally(() => {
                                    try {
                                        window._hweb_network_requests = Math.max(0, (window._hweb_network_requests || 1) - 1);
                                        window._hweb_last_activity = Date.now();
                                    } catch(e) {}
                                });
                            } catch(e) {
                                window._hweb_network_requests = Math.max(0, (window._hweb_network_requests || 1) - 1);
                                window._hweb_last_activity = Date.now();
                                return originalFetch.apply(this, arguments);
                            }
                        };
                    }
                    
                    window._hweb_xhr_overridden = true;
                } catch(e) {
                    // Fallback if we can't override network calls
                    window._hweb_xhr_overridden = true;
                }
            }
            
            // Enhanced idle checking with better timeout handling
            const checkIdle = () => {
                try {
                    const now = Date.now();
                    const timeSinceActivity = now - window._hweb_last_activity;
                    const totalElapsed = now - window._hweb_start_time;
                    
                    // Debug logging
                    if (window.console && window.console.log) {
                        console.log('Network Idle Check - Requests:', window._hweb_network_requests, 
                                  'Time since activity:', timeSinceActivity, 'Total elapsed:', totalElapsed);
                    }
                    
                    // Check if we're truly idle
                    if ((window._hweb_network_requests || 0) === 0 && timeSinceActivity >= idleTime) {
                        window._hweb_event_result = true;
                        return;
                    }
                    
                    // Check for timeout
                    if (totalElapsed >= totalTimeout) {
                        window._hweb_event_result = false;
                        return;
                    }
                    
                    // Continue checking
                    setTimeout(checkIdle, 100);
                } catch(e) {
                    window._hweb_event_result = false;
                }
            };
            
            // Start checking after initial idle time
            setTimeout(checkIdle, Math.min(idleTime, 500));
            
        })()" + std::to_string(idle_time_ms) + ", " + std::to_string(timeout_ms) + R"();
    )";
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascriptSync(networkScript);
    
    int elapsed = 0;
    const int check_interval = 200;
    
    while (elapsed < timeout_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
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
    
    debug_output("Network idle timeout - C++ side timeout");
    return false;
}

bool Browser::waitForNetworkRequest(const std::string& url_pattern, int timeout_ms) {
    debug_output("Waiting for network request matching: " + url_pattern);
    
    // Enhanced JavaScript escaping for URL patterns
    std::string escaped_pattern = url_pattern;
    size_t pos = 0;
    // Escape backslashes first
    while ((pos = escaped_pattern.find("\\", pos)) != std::string::npos) {
        escaped_pattern.replace(pos, 1, "\\\\");
        pos += 2;
    }
    // Then escape quotes
    pos = 0;
    while ((pos = escaped_pattern.find("'", pos)) != std::string::npos) {
        escaped_pattern.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    // Enhanced JavaScript to monitor for specific network requests
    std::string networkScript = R"(
        (function(pattern, totalTimeout) {
            window._hweb_event_result = undefined;
            
            // Enhanced pattern matching function
            const matchesPattern = (url) => {
                try {
                    if (!url) return false;
                    
                    // Convert URL to string if it's a URL object
                    const urlStr = (typeof url === 'string') ? url : url.toString();
                    
                    // Simple string inclusion check first
                    if (urlStr.indexOf(pattern) !== -1) {
                        return true;
                    }
                    
                    // Try regex matching if pattern looks like regex
                    if (pattern.indexOf('[') !== -1 || pattern.indexOf('*') !== -1 || pattern.indexOf('(') !== -1) {
                        try {
                            const regexPattern = pattern.replace(/\*/g, '.*');
                            const regex = new RegExp(regexPattern);
                            return regex.test(urlStr);
                        } catch(regexError) {
                            return false;
                        }
                    }
                    
                    return false;
                } catch(e) {
                    return false;
                }
            };
            
            // Set up monitoring if not already done
            if (!window._hweb_request_monitor_)" + std::to_string(timeout_ms) + R"() {
                try {
                    // Monitor XMLHttpRequest
                    const originalXHR = XMLHttpRequest.prototype.open;
                    XMLHttpRequest.prototype.open = function(method, url) {
                        try {
                            if (matchesPattern(url)) {
                                window._hweb_event_result = true;
                                if (window.console && window.console.log) {
                                    console.log('Network request detected (XHR):', url, 'matches pattern:', pattern);
                                }
                            }
                        } catch(e) {}
                        return originalXHR.apply(this, arguments);
                    };
                    
                    // Monitor fetch API
                    if (window.fetch) {
                        const originalFetch = window.fetch;
                        window.fetch = function(url) {
                            try {
                                if (matchesPattern(url)) {
                                    window._hweb_event_result = true;
                                    if (window.console && window.console.log) {
                                        console.log('Network request detected (fetch):', url, 'matches pattern:', pattern);
                                    }
                                }
                            } catch(e) {}
                            return originalFetch.apply(this, arguments);
                        };
                    }
                    
                    window._hweb_request_monitor_)" + std::to_string(timeout_ms) + R"( = true;
                } catch(setupError) {
                    if (window.console && window.console.log) {
                        console.log('Error setting up network monitoring:', setupError);
                    }
                }
            }
            
            // Timeout handler
            setTimeout(() => {
                if (window._hweb_event_result === undefined) {
                    window._hweb_event_result = false;
                    if (window.console && window.console.log) {
                        console.log('Network request timeout for pattern:', pattern);
                    }
                }
            }, totalTimeout);
            
        })(')" + escaped_pattern + "', " + std::to_string(timeout_ms) + R"();
    )";
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascriptSync(networkScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
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
    
    debug_output("Network request timeout - C++ side timeout: " + url_pattern);
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
    
    // CRITICAL FIX: Apply signal-based waiting pattern from Browser Main success
    // Clear any previous result
    executeJavascriptSync("window._hweb_event_result = undefined;");
    
    // Execute condition observer script
    std::string observerScript = setupConditionObserver(condition, timeout_ms);
    executeJavascriptSync(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        // Use shorter wait intervals for better responsiveness
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            debug_output("Element count condition met: " + selector + " " + operator_str + " " + std::to_string(expected_count));
            return true;
        } else if (result == "false") {
            debug_output("Element count condition timeout: " + selector + " " + operator_str + " " + std::to_string(expected_count));
            return false;
        }
        // If result is "undefined", continue waiting
    }
    
    debug_output("Element count condition timeout: " + selector + " " + operator_str + " " + std::to_string(expected_count));
    return false;
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
    
    // CRITICAL FIX: Apply signal-based waiting pattern from Browser Main success
    // Clear any previous result
    executeJavascriptSync("window._hweb_event_result = undefined;");
    
    // Execute condition observer script
    std::string observerScript = setupConditionObserver(condition, timeout_ms);
    executeJavascriptSync(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        // Use shorter wait intervals for better responsiveness
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            debug_output("Attribute condition met: " + selector + "[" + attribute + "='" + expected_value + "']");
            return true;
        } else if (result == "false") {
            debug_output("Attribute condition timeout: " + selector + "[" + attribute + "='" + expected_value + "']");
            return false;
        }
        // If result is "undefined", continue waiting
    }
    
    debug_output("Attribute condition timeout: " + selector + "[" + attribute + "='" + expected_value + "']");
    return false;
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
    debug_output("Initial URL: " + initial_url);
    
    int elapsed = 0;
    const int check_interval = 50; // Check every 50ms for responsiveness
    
    while (elapsed < timeout_ms) {
        // Get current URL directly
        std::string current_url = getCurrentUrl();
        
        if (route.empty()) {
            // Wait for ANY navigation change
            if (current_url != initial_url) {
                debug_output("Navigation change detected: " + initial_url + " -> " + current_url);
                return true;
            }
            
            // Also check for hash changes via JavaScript
            std::string hash_check = executeJavascriptSync(
                "(function() {"
                "  try {"
                "    var hash = window.location.hash;"
                "    var path = window.location.pathname;"
                "    return path + hash;"
                "  } catch(e) { return ''; }"
                "})()"
            );
            
            if (!hash_check.empty() && hash_check != initial_url) {
                debug_output("Hash/path change detected: " + hash_check);
                return true;
            }
        } else {
            // Wait for specific route
            if (current_url.find(route) != std::string::npos) {
                debug_output("Route found in URL: " + route + " in " + current_url);
                return true;
            }
            
            // Escape route for JavaScript
            std::string escaped_route = route;
            size_t pos = 0;
            while ((pos = escaped_route.find("'", pos)) != std::string::npos) {
                escaped_route.replace(pos, 1, "\\'");
                pos += 2;
            }
            
            // Check via JavaScript for hash/path matches (critical for history.pushState)
            std::string route_check = executeJavascriptSync(
                "(function() {"
                "  try {"
                "    var hash = window.location.hash;"
                "    var path = window.location.pathname;"
                "    var href = window.location.href;"
                "    var route = '" + escaped_route + "';"
                "    // Debug logging"
                "    if (window.console && window.console.log) {"
                "      console.log('Route check - Path:', path, 'Hash:', hash, 'Href:', href, 'Looking for:', route);"
                "    }"
                "    var result = (hash.indexOf(route) !== -1 || path.indexOf(route) !== -1 || href.indexOf(route) !== -1);"
                "    if (window.console && window.console.log) {"
                "      console.log('Route check result:', result);"
                "    }"
                "    return result;"
                "  } catch(e) {"
                "    if (window.console && window.console.log) {"
                "      console.log('Route check error:', e.message);"
                "    }"
                "    return false;"
                "  }"
                "})()"
            );
            
            if (route_check == "true" || route_check == "1") {
                debug_output("Route found via JavaScript: " + route);
                return true;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
        elapsed += check_interval;
    }
    
    debug_output("SPA navigation timeout: " + route);
    return false;
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
    
    // CRITICAL FIX: Apply signal-based waiting pattern from Browser Main success
    // Clear any previous result
    executeJavascriptSync("window._hweb_event_result = undefined;");
    
    // Execute condition observer script
    std::string observerScript = setupConditionObserver(condition, timeout_ms);
    executeJavascriptSync(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        // Use shorter wait intervals for better responsiveness
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            debug_output("Framework ready detected: " + framework);
            return true;
        } else if (result == "false") {
            debug_output("Framework ready timeout: " + framework);
            return false;
        }
        // If result is "undefined", continue waiting
    }
    
    debug_output("Framework ready timeout: " + framework);
    return false;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
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
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));  // Replace wait() with direct sleep
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
