#include "Browser.h"
#include "../Debug.h"
#include <json/json.h>
#include <iostream>

// External debug flag
extern bool g_debug;

// ========== Cookie Management ==========

void Browser::getCookiesAsync(std::function<void(std::vector<Cookie>)> callback) {
    std::string cookieJs = R"(
        (function() {
            const cookies = document.cookie.split(';').map(c => c.trim()).filter(c => c.length > 0);
            const result = [];
            
            cookies.forEach(cookie => {
                const [name, value] = cookie.split('=', 2);
                if (name && value) {
                    result.push({
                        name: name.trim(),
                        value: value.trim(),
                        domain: window.location.hostname,
                        path: '/'
                    });
                }
            });
            
            return JSON.stringify(result);
        })()
    )";
    
    std::string result = executeJavascriptSync(cookieJs);
    std::vector<Cookie> cookies;
    
    if (!result.empty() && result != "undefined") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isArray()) {
                for (const auto& item : root) {
                    Cookie cookie;
                    cookie.name = item["name"].asString();
                    cookie.value = item["value"].asString();
                    cookie.domain = item["domain"].asString();
                    cookie.path = item["path"].asString();
                    cookies.push_back(cookie);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing cookies: " << e.what() << std::endl;
        }
    }
    
    callback(cookies);
}

void Browser::setCookie(const Cookie& cookie) {
    std::string cookieStr = cookie.name + "=" + cookie.value + "; path=" + cookie.path;
    if (!cookie.domain.empty()) {
        cookieStr += "; domain=" + cookie.domain;
    }
    
    std::string js = "document.cookie = '" + cookieStr + "'; 'cookie set';";
    executeJavascriptSync(js);
}

void Browser::setCookieSafe(const Cookie& cookie) {
    try {
        setCookie(cookie);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to set cookie " << cookie.name << ": " << e.what() << std::endl;
    }
}

void Browser::clearCookies() {
    std::string clearJs = R"(
        (function() {
            document.cookie.split(";").forEach(function(c) { 
                document.cookie = c.replace(/^ +/, "").replace(/=.*/, "=;expires=" + new Date().toUTCString() + ";path=/"); 
            });
            return "cleared";
        })()
    )";
    executeJavascriptSync(clearJs);
}

// ========== Local Storage Management ==========

std::map<std::string, std::string> Browser::getLocalStorage() {
    std::map<std::string, std::string> storage;
    
    std::string storageJs = R"(
        (function() {
            try {
                const result = {};
                for (let i = 0; i < localStorage.length; i++) {
                    const key = localStorage.key(i);
                    const value = localStorage.getItem(key);
                    result[key] = value;
                }
                return JSON.stringify(result);
            } catch(e) {
                console.warn("localStorage access failed:", e.name, "- WebKit restricts localStorage on data: URLs. Consider using file:// URLs instead.");
                return "{}";
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(storageJs);
    
    if (!result.empty() && result != "undefined" && result != "{}") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isObject()) {
                for (const auto& key : root.getMemberNames()) {
                    storage[key] = root[key].asString();
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing localStorage: " << e.what() << std::endl;
        }
    }
    
    return storage;
}

void Browser::setLocalStorage(const std::map<std::string, std::string>& storage) {
    for (const auto& [key, value] : storage) {
        // Escape quotes in key and value
        std::string escaped_key = key;
        std::string escaped_value = value;
        
        size_t pos = 0;
        while ((pos = escaped_key.find("'", pos)) != std::string::npos) {
            escaped_key.replace(pos, 1, "\\'");
            pos += 2;
        }
        
        pos = 0;
        while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
            escaped_value.replace(pos, 1, "\\'");
            pos += 2;
        }
        
        std::string js = "try { localStorage.setItem('" + escaped_key + "', '" + escaped_value + "'); } catch(e) { " +
                         "'localStorage error: ' + e.name + ' - Note: WebKit restricts localStorage on data: URLs. Consider using file:// URLs instead.'; }";
        std::string result = executeJavascriptSync(js);
        
        // If we get an error result and current URL is a data: URL, warn the user
        if (!result.empty() && result.find("localStorage error") != std::string::npos) {
            std::string currentUrl = getCurrentUrl();
            if (currentUrl.find("data:") == 0) {
                debug_output("WARNING: localStorage operation failed on data: URL. WebKit restricts storage on data: URLs. Consider using file:// URLs for full storage functionality.");
            }
        }
    }
}

// ========== Session Storage Management ==========

std::map<std::string, std::string> Browser::getSessionStorage() {
    std::map<std::string, std::string> storage;
    
    std::string storageJs = R"(
        (function() {
            try {
                const result = {};
                for (let i = 0; i < sessionStorage.length; i++) {
                    const key = sessionStorage.key(i);
                    const value = sessionStorage.getItem(key);
                    result[key] = value;
                }
                return JSON.stringify(result);
            } catch(e) {
                console.warn("sessionStorage access failed:", e.name, "- WebKit restricts sessionStorage on data: URLs. Consider using file:// URLs instead.");
                return "{}";
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(storageJs);
    
    if (!result.empty() && result != "undefined" && result != "{}") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isObject()) {
                for (const auto& key : root.getMemberNames()) {
                    storage[key] = root[key].asString();
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing sessionStorage: " << e.what() << std::endl;
        }
    }
    
    return storage;
}

void Browser::setSessionStorage(const std::map<std::string, std::string>& storage) {
    for (const auto& [key, value] : storage) {
        // Escape quotes in key and value
        std::string escaped_key = key;
        std::string escaped_value = value;
        
        size_t pos = 0;
        while ((pos = escaped_key.find("'", pos)) != std::string::npos) {
            escaped_key.replace(pos, 1, "\\'");
            pos += 2;
        }
        
        pos = 0;
        while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
            escaped_value.replace(pos, 1, "\\'");
            pos += 2;
        }
        
        std::string js = "try { sessionStorage.setItem('" + escaped_key + "', '" + escaped_value + "'); } catch(e) { " +
                         "'sessionStorage error: ' + e.name + ' - Note: WebKit restricts sessionStorage on data: URLs. Consider using file:// URLs instead.'; }";
        std::string result = executeJavascriptSync(js);
        
        // If we get an error result and current URL is a data: URL, warn the user
        if (!result.empty() && result.find("sessionStorage error") != std::string::npos) {
            std::string currentUrl = getCurrentUrl();
            if (currentUrl.find("data:") == 0) {
                debug_output("WARNING: sessionStorage operation failed on data: URL. WebKit restricts storage on data: URLs. Consider using file:// URLs for full storage functionality.");
            }
        }
    }
}

// ========== Storage Clearing Methods ==========

void Browser::clearLocalStorage() {
    executeJavascriptSync("try { localStorage.clear(); } catch(e) { 'localStorage clear error'; }");
}

void Browser::clearSessionStorage() {
    executeJavascriptSync("try { sessionStorage.clear(); } catch(e) { 'sessionStorage clear error'; }");
}

void Browser::clearAllStorage() {
    clearLocalStorage();
    clearSessionStorage();
}
