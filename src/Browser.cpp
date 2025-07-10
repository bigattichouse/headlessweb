#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <unistd.h>
#include <glib.h>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>

// Callback for JavaScript evaluation
static void js_eval_callback(GObject* object, GAsyncResult* res, gpointer user_data) {
    GError* error = NULL;
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), res, &error);
    
    Browser* browser_instance = static_cast<Browser*>(g_object_get_data(G_OBJECT(object), "browser-instance"));
    
    if (error) {
        std::cerr << "JavaScript error: " << error->message << std::endl;
        g_error_free(error);
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    } else if (value) {
        if (user_data) {
            if (jsc_value_is_string(value)) {
                char* str_value = jsc_value_to_string(value);
                if (str_value) {
                    *(static_cast<std::string*>(user_data)) = str_value;
                    g_free(str_value);
                } else {
                    *(static_cast<std::string*>(user_data)) = "";
                }
            } else if (jsc_value_is_number(value)) {
                *(static_cast<std::string*>(user_data)) = std::to_string(jsc_value_to_double(value));
            } else if (jsc_value_is_boolean(value)) {
                *(static_cast<std::string*>(user_data)) = jsc_value_to_boolean(value) ? "true" : "false";
            } else if (jsc_value_is_null(value)) {
                *(static_cast<std::string*>(user_data)) = "null";
            } else if (jsc_value_is_undefined(value)) {
                *(static_cast<std::string*>(user_data)) = "undefined";
            } else {
                *(static_cast<std::string*>(user_data)) = "";
            }
        }
        g_object_unref(value);
    } else {
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    }
    
    if (browser_instance) {
        browser_instance->operation_completed = true;
    }
}

// Callback for load-changed signal
static void load_changed_callback(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        Browser* browser_instance = static_cast<Browser*>(user_data);
        if (browser_instance) {
            browser_instance->operation_completed = true;
        }
    }
}

Browser::Browser() : cookieManager(nullptr), dataManager(nullptr), operation_completed(false) {
    gtk_init();
    
    // Initialize with a proper data manager for cookie/storage persistence
    std::string home = std::getenv("HOME");
    sessionDataPath = home + "/.hweb-poc/webkit-data";
    std::filesystem::create_directories(sessionDataPath);
    
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    if (!webView) {
        std::cerr << "Failed to create WebKit web view" << std::endl;
        exit(1);
    }
    
    // Create and apply settings
    WebKitSettings* settings = webkit_settings_new();
    webkit_settings_set_enable_media(settings, FALSE);
    webkit_settings_set_enable_media_stream(settings, FALSE);
    webkit_settings_set_enable_webaudio(settings, FALSE);
    webkit_settings_set_enable_javascript(settings, TRUE);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_page_cache(settings, TRUE);
    webkit_settings_set_enable_html5_local_storage(settings, TRUE);
    webkit_settings_set_enable_html5_database(settings, TRUE);
    
    webkit_web_view_set_settings(webView, settings);
    
    // Get the network session for cookie management
    WebKitNetworkSession* session = webkit_web_view_get_network_session(webView);
    if (session) {
        cookieManager = webkit_network_session_get_cookie_manager(session);
        
        // Set persistent cookie storage
        std::string cookieFile = sessionDataPath + "/cookies.txt";
        webkit_cookie_manager_set_persistent_storage(cookieManager, 
            cookieFile.c_str(), WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
    }
    
    window = gtk_window_new();
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(webView));
    
    // Set window to be offscreen for headless operation
    gtk_widget_set_visible(window, FALSE);
    
    // Store browser instance in the webView for callbacks
    g_object_set_data(G_OBJECT(webView), "browser-instance", this);
    
    g_object_unref(settings);
}

Browser::~Browser() {
    // Don't do anything here - let GTK handle cleanup
}

void Browser::loadUri(const std::string& uri) {
    operation_completed = false;
    g_signal_connect(webView, "load-changed", G_CALLBACK(load_changed_callback), this);
    webkit_web_view_load_uri(webView, uri.c_str());
}

std::string Browser::getCurrentUrl() {
    const gchar* uri = webkit_web_view_get_uri(webView);
    return uri ? std::string(uri) : "";
}

std::string Browser::getPageTitle() {
    const gchar* title = webkit_web_view_get_title(webView);
    return title ? std::string(title) : "";
}

void Browser::goBack() {
    webkit_web_view_go_back(webView);
}

void Browser::goForward() {
    webkit_web_view_go_forward(webView);
}

void Browser::reload() {
    webkit_web_view_reload(webView);
}

void Browser::executeJavascript(const std::string& script, std::string* result) {
    operation_completed = false;
    
    if (result) {
        result->clear();
    }
    
    if (script.empty()) {
        std::cerr << "Warning: Empty JavaScript script" << std::endl;
        if (result) {
            *result = "";
        }
        operation_completed = true;
        return;
    }
    
    if (!webView) {
        std::cerr << "Error: WebView is null" << std::endl;
        if (result) {
            *result = "";
        }
        operation_completed = true;
        return;
    }
    
    webkit_web_view_evaluate_javascript(
        webView, 
        script.c_str(), 
        -1,
        NULL, 
        NULL, 
        NULL, 
        js_eval_callback, 
        result
    );
}

bool Browser::waitForJavaScriptCompletion(int timeout_ms) {
    int elapsed_time = 0;
    while (!isOperationCompleted() && elapsed_time < timeout_ms) {
        g_main_context_iteration(g_main_context_default(), FALSE);
        g_usleep(10 * 1000);
        elapsed_time += 10;
    }
    return operation_completed;
}

std::string Browser::executeJavascriptSync(const std::string& script) {
    if (script.empty() || !webView) {
        return "";
    }
    
    // Allocate result on heap to avoid stack issues
    std::string* result = new std::string();
    
    try {
        executeJavascript(script, result);
        if (!waitForJavaScriptCompletion(5000)) {
            std::cerr << "JavaScript execution timeout for: " << script.substr(0, 50) << "..." << std::endl;
            delete result;
            return "";
        }
        
        // Make a copy of the result before deleting
        std::string final_result;
        if (result && !result->empty()) {
            // Ensure we don't have a ridiculously long result
            if (result->length() > 100000) {  // 100KB limit
                final_result = result->substr(0, 100000);
            } else {
                final_result = *result;
            }
        }
        
        delete result;
        return final_result;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in JavaScript execution: " << e.what() << std::endl;
        delete result;
        return "";
    }
}

void Browser::restoreSession(const Session& session) {
    try {
        // Set user agent first if present
        if (!session.getUserAgent().empty()) {
            setUserAgent(session.getUserAgent());
            wait(100); // Small delay for user agent to take effect
        }
        
        // Navigate to current URL if present
        if (!session.getCurrentUrl().empty()) {
            std::cerr << "Debug: Loading URL: " << session.getCurrentUrl() << std::endl;
            loadUri(session.getCurrentUrl());
            
            // Wait for load with longer timeout
            if (!waitForJavaScriptCompletion(15000)) {
                std::cerr << "Warning: Page load timeout during session restore" << std::endl;
                return; // Don't try to restore state on failed page load
            }
            
            // Wait for page to be ready
            wait(2000); // Longer wait for stability
            
            // Verify we can execute JavaScript
            std::string testResult = executeJavascriptSync("(function() { return 'test'; })()");
            if (testResult != "test") {
                std::cerr << "Warning: JavaScript execution not working properly" << std::endl;
                return;
            }
            
            std::cerr << "Debug: Page loaded successfully" << std::endl;
        }
        
        // Only restore state if page loaded successfully
        std::string readyState = executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return 'error'; } })()");
        if (readyState != "complete" && readyState != "interactive") {
            std::cerr << "Warning: Page not ready for state restoration (state: " << readyState << ")" << std::endl;
            return;
        }
        
        // Restore state step by step with error handling
        std::cerr << "Debug: Starting state restoration..." << std::endl;
        
        // Cookies
        try {
            if (!session.getCookies().empty()) {
                for (const auto& cookie : session.getCookies()) {
                    setCookie(cookie);
                }
                wait(500);
                std::cerr << "Debug: Restored " << session.getCookies().size() << " cookies" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore cookies: " << e.what() << std::endl;
        }
        
        // Storage
        try {
            if (!session.getLocalStorage().empty()) {
                setLocalStorage(session.getLocalStorage());
                wait(500);
                std::cerr << "Debug: Restored localStorage" << std::endl;
            }
            
            if (!session.getSessionStorage().empty()) {
                setSessionStorage(session.getSessionStorage());
                wait(500);
                std::cerr << "Debug: Restored sessionStorage" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore storage: " << e.what() << std::endl;
        }
        
        // Form state
        try {
            if (!session.getFormFields().empty()) {
                restoreFormState(session.getFormFields());
                wait(500);
                std::cerr << "Debug: Restored form state" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore form state: " << e.what() << std::endl;
        }
        
        // Scroll positions
        try {
            if (!session.getAllScrollPositions().empty()) {
                restoreScrollPositions(session.getAllScrollPositions());
                wait(500);
                std::cerr << "Debug: Restored scroll positions" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore scroll positions: " << e.what() << std::endl;
        }
        
        // Final wait for everything to settle
        wait(1000);
        std::cerr << "Debug: Session restoration complete" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in session restoration: " << e.what() << std::endl;
    }
}

void Browser::updateSessionState(Session& session) {
    try {
        // Always update current URL first (this should never fail)
        session.setCurrentUrl(getCurrentUrl());
        
        // Try a simple JavaScript test first
        std::string testResult = executeJavascriptSync("(function() { try { return 'alive'; } catch(e) { return 'dead'; } })()");
        if (testResult != "alive") {
            std::cerr << "Warning: JavaScript execution not working, skipping state extraction" << std::endl;
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
            
            // Cookies - try but don't fail if it doesn't work
            try {
                getCookiesAsync([&session](std::vector<Cookie> cookies) {
                    session.setCookies(cookies);
                });
                waitForJavaScriptCompletion(1000); // Shorter timeout
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract cookies: " << e.what() << std::endl;
            }
            
            // Storage - with error handling
            try {
                session.setLocalStorage(getLocalStorage());
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract localStorage: " << e.what() << std::endl;
            }
            
            try {
                session.setSessionStorage(getSessionStorage());
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract sessionStorage: " << e.what() << std::endl;
            }
            
            // Form state - this was causing crashes, so be extra careful
            try {
                // Test if we can query form elements first
                std::string formTest = executeJavascriptSync("(function() { try { return document.querySelectorAll('input, textarea, select').length.toString(); } catch(e) { return 'error'; } })()");
                if (formTest != "error" && !formTest.empty()) {
                    auto formFields = extractFormState();
                    session.setFormFields(formFields);
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract form state: " << e.what() << std::endl;
            }
            
            // Active elements
            try {
                session.setActiveElements(extractActiveElements());
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract active elements: " << e.what() << std::endl;
            }
            
            // Scroll positions
            try {
                auto scrollPositions = extractAllScrollPositions();
                for (const auto& [selector, pos] : scrollPositions) {
                    session.setScrollPosition(selector, pos.first, pos.second);
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract scroll positions: " << e.what() << std::endl;
            }
            
            // Custom state - skip for now to avoid issues
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

void Browser::getCookiesAsync(std::function<void(std::vector<Cookie>)> callback) {
    std::string js = R"(
        (function() {
            try {
                var cookies = document.cookie.split('; ');
                var result = [];
                for (var i = 0; i < cookies.length; i++) {
                    if (cookies[i].length > 0) {
                        var parts = cookies[i].split('=');
                        if (parts.length >= 2) {
                            result.push({
                                name: parts[0],
                                value: parts.slice(1).join('='),
                                domain: window.location.hostname,
                                path: '/'
                            });
                        }
                    }
                }
                return JSON.stringify(result);
            } catch(e) {
                return '[]';
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    std::vector<Cookie> cookies;
    
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(result, root) && root.isArray()) {
        for (const auto& cookieJson : root) {
            Cookie cookie;
            cookie.name = cookieJson.get("name", "").asString();
            cookie.value = cookieJson.get("value", "").asString();
            cookie.domain = cookieJson.get("domain", "").asString();
            cookie.path = cookieJson.get("path", "/").asString();
            cookie.secure = false;
            cookie.httpOnly = false;
            cookie.expires = -1;
            cookies.push_back(cookie);
        }
    }
    
    callback(cookies);
    operation_completed = true;
}

void Browser::setCookie(const Cookie& cookie) {
    std::stringstream js;
    js << "(function() { try { document.cookie = \"" << cookie.name << "=" << cookie.value;
    
    if (!cookie.domain.empty()) {
        js << "; domain=" << cookie.domain;
    }
    
    js << "; path=" << cookie.path;
    
    if (cookie.secure) {
        js << "; secure";
    }
    
    if (cookie.httpOnly) {
        js << "; httpOnly";
    }
    
    if (cookie.expires > 0) {
        js << "; expires=" << cookie.expires;
    }
    
    js << "\"; return 'set'; } catch(e) { return 'error'; } })()";
    
    executeJavascript(js.str());
    waitForJavaScriptCompletion(500);
}

std::map<std::string, std::string> Browser::getLocalStorage() {
    std::string js = R"(
        (function() {
            try {
                var result = {};
                for (var i = 0; i < localStorage.length; i++) {
                    var key = localStorage.key(i);
                    result[key] = localStorage.getItem(key);
                }
                return JSON.stringify(result);
            } catch(e) {
                return '{}';
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    std::map<std::string, std::string> storage;
    
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(result, root) && root.isObject()) {
        for (const auto& key : root.getMemberNames()) {
            storage[key] = root[key].asString();
        }
    }
    
    return storage;
}

void Browser::setLocalStorage(const std::map<std::string, std::string>& storage) {
    executeJavascript("(function() { try { localStorage.clear(); return 'cleared'; } catch(e) { return 'error'; } })()");
    waitForJavaScriptCompletion(500);
    
    for (const auto& [key, value] : storage) {
        std::stringstream js;
        js << "(function() { try { localStorage.setItem('" << key << "', '" << value << "'); return 'set'; } catch(e) { return 'error'; } })()";
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::map<std::string, std::string> Browser::getSessionStorage() {
    std::string js = R"(
        (function() {
            try {
                var result = {};
                for (var i = 0; i < sessionStorage.length; i++) {
                    var key = sessionStorage.key(i);
                    result[key] = sessionStorage.getItem(key);
                }
                return JSON.stringify(result);
            } catch(e) {
                return '{}';
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    std::map<std::string, std::string> storage;
    
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(result, root) && root.isObject()) {
        for (const auto& key : root.getMemberNames()) {
            storage[key] = root[key].asString();
        }
    }
    
    return storage;
}

void Browser::setSessionStorage(const std::map<std::string, std::string>& storage) {
    executeJavascript("(function() { try { sessionStorage.clear(); return 'cleared'; } catch(e) { return 'error'; } })()");
    waitForJavaScriptCompletion(500);
    
    for (const auto& [key, value] : storage) {
        std::stringstream js;
        js << "(function() { try { sessionStorage.setItem('" << key << "', '" << value << "'); return 'set'; } catch(e) { return 'error'; } })()";
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::vector<FormField> Browser::extractFormState() {
    std::string js = R"(
        (function() {
            try {
                var fields = [];
                var elements = document.querySelectorAll('input, textarea, select');
                
                for (var i = 0; i < elements.length; i++) {
                    var el = elements[i];
                    var field = {
                        selector: '',
                        name: el.name || '',
                        id: el.id || '',
                        type: el.type || el.tagName.toLowerCase(),
                        value: '',
                        checked: false
                    };
                    
                    // Build a unique selector
                    if (el.id) {
                        field.selector = '#' + el.id;
                    } else if (el.name) {
                        field.selector = '[name="' + el.name + '"]';
                    } else {
                        field.selector = el.tagName.toLowerCase() + ':nth-of-type(' + (i + 1) + ')';
                    }
                    
                    // Get value based on type - ensure strings
                    if (el.type === 'checkbox' || el.type === 'radio') {
                        field.checked = el.checked;
                        field.value = String(el.value || '');
                    } else if (el.tagName === 'SELECT') {
                        field.value = String(el.value || '');
                    } else {
                        field.value = String(el.value || '');
                    }
                    
                    fields.push(field);
                }
                
                return JSON.stringify(fields);
            } catch(e) {
                console.log('extractFormState error:', e);
                return '[]';
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    std::vector<FormField> fields;
    
    if (result.empty() || result == "[]") {
        return fields; // Return empty vector if no result
    }
    
    try {
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(result, root) && root.isArray()) {
            for (const auto& fieldJson : root) {
                FormField field;
                field.selector = fieldJson.get("selector", "").asString();
                field.name = fieldJson.get("name", "").asString();
                field.id = fieldJson.get("id", "").asString();
                field.type = fieldJson.get("type", "").asString();
                field.value = fieldJson.get("value", "").asString();
                field.checked = fieldJson.get("checked", false).asBool();
                fields.push_back(field);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing form state JSON: " << e.what() << std::endl;
        // Return empty vector instead of crashing
    }
    
    return fields;
}

void Browser::restoreFormState(const std::vector<FormField>& fields) {
    for (const auto& field : fields) {
        std::stringstream js;
        
        if (field.type == "checkbox" || field.type == "radio") {
            js << "(function() { try { var el = document.querySelector('" << field.selector << "'); ";
            js << "if (el) { el.checked = " << (field.checked ? "true" : "false") << "; ";
            js << "el.dispatchEvent(new Event('change', { bubbles: true })); return 'set'; } ";
            js << "return 'not_found'; } catch(e) { return 'error'; } })()";
        } else {
            js << "(function() { try { var el = document.querySelector('" << field.selector << "'); ";
            js << "if (el) { el.value = '" << field.value << "'; ";
            js << "el.dispatchEvent(new Event('input', { bubbles: true })); ";
            js << "el.dispatchEvent(new Event('change', { bubbles: true })); return 'set'; } ";
            js << "return 'not_found'; } catch(e) { return 'error'; } })()";
        }
        
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::set<std::string> Browser::extractActiveElements() {
    std::string js = R"(
        (function() {
            try {
                var active = [];
                
                // Checked checkboxes and radios
                document.querySelectorAll('input[type="checkbox"]:checked, input[type="radio"]:checked').forEach(function(el) {
                    if (el.id) {
                        active.push('#' + el.id);
                    } else if (el.name) {
                        active.push('[name="' + el.name + '"][value="' + el.value + '"]');
                    }
                });
                
                // Selected options
                document.querySelectorAll('option:checked').forEach(function(el) {
                    if (el.parentElement.id) {
                        active.push('#' + el.parentElement.id + ' option[value="' + el.value + '"]');
                    }
                });
                
                return JSON.stringify(active);
            } catch(e) {
                return '[]';
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    std::set<std::string> elements;
    
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(result, root) && root.isArray()) {
        for (const auto& element : root) {
            elements.insert(element.asString());
        }
    }
    
    return elements;
}

void Browser::restoreActiveElements(const std::set<std::string>& elements) {
    for (const auto& selector : elements) {
        std::stringstream js;
        js << "(function() { try { var el = document.querySelector('" << selector << "'); ";
        js << "if (el) { ";
        js << "  if (el.type === 'checkbox' || el.type === 'radio') { el.checked = true; } ";
        js << "  else if (el.tagName === 'OPTION') { el.selected = true; } ";
        js << "  else { el.classList.add('active'); } ";
        js << "  el.dispatchEvent(new Event('change', { bubbles: true })); ";
        js << "  return 'set'; ";
        js << "} return 'not_found'; } catch(e) { return 'error'; } })()";
        
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::string Browser::extractPageHash() {
    return executeJavascriptSync("(function() { try { return window.location.hash; } catch(e) { return ''; } })()");
}

std::string Browser::extractDocumentReadyState() {
    return executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return ''; } })()");
}

std::map<std::string, std::pair<int, int>> Browser::extractAllScrollPositions() {
    std::string js = R"(
        (function() {
            try {
                var positions = {};
                
                // Window scroll
                positions['window'] = [window.pageXOffset || 0, window.pageYOffset || 0];
                
                // Find scrollable elements with actual scroll
                var elements = document.querySelectorAll('*');
                for (var i = 0; i < elements.length; i++) {
                    var el = elements[i];
                    if (el.scrollHeight > el.clientHeight || el.scrollWidth > el.clientWidth) {
                        var selector = '';
                        if (el.id) {
                            selector = '#' + el.id;
                        } else if (el.className) {
                            selector = '.' + el.className.split(' ')[0];
                        }
                        
                        if (selector && (el.scrollTop > 0 || el.scrollLeft > 0)) {
                            positions[selector] = [el.scrollLeft || 0, el.scrollTop || 0];
                        }
                    }
                }
                
                return JSON.stringify(positions);
            } catch(e) {
                return '{"window":[0,0]}';
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    std::map<std::string, std::pair<int, int>> positions;
    
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(result, root) && root.isObject()) {
        for (const auto& key : root.getMemberNames()) {
            if (root[key].isArray() && root[key].size() >= 2) {
                positions[key] = {root[key][0].asInt(), root[key][1].asInt()};
            }
        }
    }
    
    return positions;
}

void Browser::restoreScrollPositions(const std::map<std::string, std::pair<int, int>>& positions) {
    for (const auto& [selector, pos] : positions) {
        std::stringstream js;
        
        if (selector == "window") {
            js << "(function() { try { window.scrollTo(" << pos.first << ", " << pos.second << "); return 'set'; } catch(e) { return 'error'; } })()";
        } else {
            js << "(function() { try { var el = document.querySelector('" << selector << "'); ";
            js << "if (el) { el.scrollLeft = " << pos.first << "; el.scrollTop = " << pos.second << "; return 'set'; } ";
            js << "return 'not_found'; } catch(e) { return 'error'; } })()";
        }
        
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(500);
}

bool Browser::waitForPageReady(const Session& session) {
    // Wait for basic document ready
    waitForJsCondition("document.readyState === 'complete'", 10000);
    
    // Wait for any custom ready conditions
    for (const auto& condition : session.getReadyConditions()) {
        switch (condition.type) {
            case PageReadyCondition::SELECTOR:
                if (!waitForSelector(condition.value, condition.timeout)) {
                    std::cerr << "Warning: Ready selector not found: " << condition.value << std::endl;
                }
                break;
                
            case PageReadyCondition::JS_EXPRESSION:
                if (!waitForJsCondition(condition.value, condition.timeout)) {
                    std::cerr << "Warning: Ready condition not met: " << condition.value << std::endl;
                }
                break;
                
            case PageReadyCondition::CUSTOM:
                executeJavascript(condition.value);
                waitForJavaScriptCompletion(condition.timeout);
                break;
        }
    }
    
    // Additional wait for dynamic content
    wait(500);
    
    return true;
}

bool Browser::waitForJsCondition(const std::string& condition, int timeout_ms) {
    int elapsed_time = 0;
    std::string js_check = "(function() { try { return " + condition + "; } catch(e) { return false; } })()";
    
    while (elapsed_time < timeout_ms) {
        std::string result = executeJavascriptSync(js_check);
        if (result == "true") {
            return true;
        }
        
        wait(100);
        elapsed_time += 100;
    }
    
    return false;
}

Json::Value Browser::extractCustomState(const std::map<std::string, std::string>& extractors) {
    Json::Value result;
    
    for (const auto& [name, jsCode] : extractors) {
        std::string extractedValue = executeJavascriptSync(jsCode);
        
        Json::Value parsed;
        Json::Reader reader;
        if (reader.parse(extractedValue, parsed)) {
            result[name] = parsed;
        } else {
            result[name] = extractedValue;
        }
    }
    
    return result;
}

void Browser::restoreCustomState(const std::map<std::string, Json::Value>& state) {
    for (const auto& [key, value] : state) {
        std::cout << "Would restore custom state: " << key << std::endl;
    }
}

bool Browser::executeActionSequence(const std::vector<Session::RecordedAction>& actions) {
    for (const auto& action : actions) {
        if (action.delay > 0) {
            wait(action.delay);
        }
        
        bool success = false;
        
        if (action.type == "click") {
            success = clickElement(action.selector);
        } else if (action.type == "type") {
            success = fillInput(action.selector, action.value);
        } else if (action.type == "submit") {
            success = submitForm(action.selector);
        } else if (action.type == "select") {
            success = selectOption(action.selector, action.value);
        } else if (action.type == "check") {
            success = checkElement(action.selector);
        } else if (action.type == "uncheck") {
            success = uncheckElement(action.selector);
        } else if (action.type == "focus") {
            success = focusElement(action.selector);
        } else if (action.type == "wait") {
            success = waitForSelector(action.selector, 10000);
        } else if (action.type == "wait-nav") {
            success = waitForNavigation(10000);
        }
        
        if (!success) {
            std::cerr << "Action failed: " << action.type << " on " << action.selector << std::endl;
            return false;
        }
    }
    
    return true;
}

bool Browser::selectOption(const std::string& selector, const std::string& value) {
    std::stringstream js;
    js << "(function() { ";
    js << "  try { ";
    js << "    var select = document.querySelector('" << selector << "'); ";
    js << "    if (select) { ";
    js << "      select.value = '" << value << "'; ";
    js << "      select.dispatchEvent(new Event('change', { bubbles: true })); ";
    js << "      return true; ";
    js << "    } ";
    js << "    return false; ";
    js << "  } catch(e) { return false; } ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::checkElement(const std::string& selector) {
    std::stringstream js;
    js << "(function() { ";
    js << "  try { ";
    js << "    var el = document.querySelector('" << selector << "'); ";
    js << "    if (el && (el.type === 'checkbox' || el.type === 'radio')) { ";
    js << "      el.checked = true; ";
    js << "      el.dispatchEvent(new Event('change', { bubbles: true })); ";
    js << "      return true; ";
    js << "    } ";
    js << "    return false; ";
    js << "  } catch(e) { return false; } ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::uncheckElement(const std::string& selector) {
    std::stringstream js;
    js << "(function() { ";
    js << "  try { ";
    js << "    var el = document.querySelector('" << selector << "'); ";
    js << "    if (el && (el.type === 'checkbox' || el.type === 'radio')) { ";
    js << "      el.checked = false; ";
    js << "      el.dispatchEvent(new Event('change', { bubbles: true })); ";
    js << "      return true; ";
    js << "    } ";
    js << "    return false; ";
    js << "  } catch(e) { return false; } ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::focusElement(const std::string& selector) {
    std::stringstream js;
    js << "(function() { ";
    js << "  try { ";
    js << "    var el = document.querySelector('" << selector << "'); ";
    js << "    if (el) { ";
    js << "      el.focus(); ";
    js << "      return true; ";
    js << "    } ";
    js << "    return false; ";
    js << "  } catch(e) { return false; } ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::elementExists(const std::string& selector) {
    std::string js = "(function() { try { return document.querySelector('" + selector + "') !== null; } catch(e) { return false; } })()";
    std::string result = executeJavascriptSync(js);
    return result == "true";
}

int Browser::countElements(const std::string& selector) {
    std::string js = "(function() { try { return document.querySelectorAll('" + selector + "').length; } catch(e) { return 0; } })()";
    std::string result = executeJavascriptSync(js);
    try {
        return std::stoi(result);
    } catch (...) {
        return 0;
    }
}

std::string Browser::getElementHtml(const std::string& selector) {
    std::stringstream js;
    js << "(function() { ";
    js << "  try { ";
    js << "    var el = document.querySelector('" << selector << "'); ";
    js << "    return el ? el.outerHTML : ''; ";
    js << "  } catch(e) { return ''; } ";
    js << "})()";
    
    return executeJavascriptSync(js.str());
}

void Browser::takeScreenshot(const std::string& filename) {
    std::cout << "Screenshot functionality not implemented in this version" << std::endl;
}

std::string Browser::getPageSource() {
    return executeJavascriptSync("(function() { try { return document.documentElement.outerHTML; } catch(e) { return ''; } })()");
}

std::string Browser::getInnerText(const std::string& selector) {
    std::string escaped_selector;
    escaped_selector.reserve(selector.length());
    for (char c : selector) {
        if (c == '\\') {
            escaped_selector += "\\\\";
        } else if (c == '\'') {
            escaped_selector += "\\'";
        } else {
            escaped_selector += c;
        }
    }
    
    std::string text_content_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + escaped_selector + "'); "
        "    if (!element) return ''; "
        "    var text = (element.innerText || element.textContent || '').trim(); "
        "    return text; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(text_content_script);
    
    std::string cleaned;
    cleaned.reserve(result.length());
    for (char c : result) {
        if (c >= 32 && c <= 126) {
            cleaned += c;
        } else if (c == '\n' || c == '\r' || c == '\t') {
            cleaned += c;
        } else if ((unsigned char)c >= 128) {
            cleaned += c;
        } else {
            cleaned += ' ';
        }
    }
    
    return cleaned;
}

std::string Browser::getFirstNonEmptyText(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var elements = document.querySelectorAll('" + selector + "'); "
        "    for (var i = 0; i < elements.length; i++) { "
        "      var text = elements[i].innerText || elements[i].textContent || ''; "
        "      if (text.trim()) { "
        "        return text.trim(); "
        "      } "
        "    } "
        "    return ''; "
        "  } catch(e) { return ''; } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

bool Browser::fillInput(const std::string& selector, const std::string& value) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.value = '" + value + "'; "
        "      element.dispatchEvent(new Event('input', { bubbles: true })); "
        "      element.dispatchEvent(new Event('change', { bubbles: true })); "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { return false; } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::clickElement(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.click(); "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { return false; } "
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
        "  } catch(e) { return false; } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::waitForNavigation(int timeout_ms) {
    std::string initial_url = getCurrentUrl();
    int elapsed_time = 0;
    
    while (elapsed_time < timeout_ms) {
        wait(100);
        elapsed_time += 100;
        
        std::string current_url = getCurrentUrl();
        if (current_url != initial_url && !current_url.empty()) {
            wait(500);
            return true;
        }
    }
    
    return false;
}

bool Browser::searchForm(const std::string& query) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var searchInputs = document.querySelectorAll('input[name*=search], input[type=search], input[placeholder*=search i], input[placeholder*=Search]'); "
        "    var searchButtons = document.querySelectorAll('button[name*=search], input[type=submit], button[type=submit]'); "
        "    "
        "    if (searchInputs.length > 0) { "
        "      searchInputs[0].value = '" + query + "'; "
        "      searchInputs[0].dispatchEvent(new Event('input', { bubbles: true })); "
        "      searchInputs[0].dispatchEvent(new Event('change', { bubbles: true })); "
        "      "
        "      if (searchButtons.length > 0) { "
        "        searchButtons[0].click(); "
        "      } else { "
        "        searchInputs[0].form && searchInputs[0].form.submit(); "
        "      } "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { return false; } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

std::string Browser::getAttribute(const std::string& selector, const std::string& attribute) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element ? (element.getAttribute('" + attribute + "') || '') : ''; "
        "  } catch(e) { return ''; } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

void Browser::setViewport(int width, int height) {
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
}

void Browser::setScrollPosition(int x, int y) {
    std::stringstream js;
    js << "(function() { try { window.scrollTo(" << x << ", " << y << "); return 'done'; } catch(e) { return 'error'; } })()";
    executeJavascript(js.str());
    waitForJavaScriptCompletion(500);
}

std::pair<int, int> Browser::getScrollPosition() {
    std::string js = "(function() { try { return JSON.stringify({x: window.pageXOffset, y: window.pageYOffset}); } catch(e) { return '{\"x\":0,\"y\":0}'; } })()";
    std::string result = executeJavascriptSync(js);
    
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(result, root)) {
        return {root.get("x", 0).asInt(), root.get("y", 0).asInt()};
    }
    
    return {0, 0};
}

void Browser::setUserAgent(const std::string& userAgent) {
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_user_agent(settings, userAgent.c_str());
}

void Browser::clearCookies() {
    std::string js = R"(
        (function() {
            try {
                var cookies = document.cookie.split(';');
                
                for (var i = 0; i < cookies.length; i++) {
                    var cookie = cookies[i];
                    var eqPos = cookie.indexOf("=");
                    var name = eqPos > -1 ? cookie.substr(0, eqPos).trim() : cookie.trim();
                    
                    document.cookie = name + "=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/";
                    document.cookie = name + "=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=" + window.location.hostname;
                    document.cookie = name + "=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=." + window.location.hostname;
                }
                
                return 'Cookies cleared';
            } catch(e) {
                return 'Error clearing cookies';
            }
        })()
    )";
    
    executeJavascript(js);
    waitForJavaScriptCompletion(500);
}

bool Browser::waitForSelector(const std::string& selector, int timeout_ms) {
    int elapsed_time = 0;
    std::string js_check_script = "(function() { try { return document.querySelector('" + selector + "') !== null; } catch(e) { return false; } })()";

    while (elapsed_time < timeout_ms) {
        std::string js_result_str;
        operation_completed = false;
        webkit_web_view_evaluate_javascript(webView, js_check_script.c_str(), -1, NULL, NULL, NULL, js_eval_callback, &js_result_str);
        
        int iteration_time = 0;
        while (!operation_completed && iteration_time < 1000) {
            g_main_context_iteration(g_main_context_default(), FALSE);
            g_usleep(100 * 1000);
            iteration_time += 100;
        }

        if (js_result_str == "true") {
            return true;
        }
        elapsed_time += iteration_time;
    }
    return false;
}

bool Browser::waitForText(const std::string& text, int timeout_ms) {
    int elapsed_time = 0;
    std::string escaped_text = text;
    size_t pos = escaped_text.find("'");
    while (pos != std::string::npos) {
        escaped_text.replace(pos, 1, "\\'");
        pos = escaped_text.find("'", pos + 2);
    }

    std::string js_check_script = "(function() { try { return document.body.innerText.includes('" + escaped_text + "'); } catch(e) { return false; } })()";
    
    while (elapsed_time < timeout_ms) {
        std::string result = executeJavascriptSync(js_check_script);
        if (result == "true") {
            return true;
        }
        
        wait(100);
        elapsed_time += 100;
    }
    
    return false;
}

bool Browser::waitForElementWithContent(const std::string& selector, int timeout_ms) {
    std::string js_check = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element && (element.innerText.trim() || element.textContent.trim()) ? true : false; "
        "  } catch(e) { return false; } "
        "})()";
    
    return waitForJsCondition(js_check, timeout_ms);
}

void Browser::wait(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

bool Browser::isOperationCompleted() const {
    return operation_completed;
}

// Enhanced methods for better reliability

bool Browser::isPageLoaded() const {
    std::string readyState = const_cast<Browser*>(this)->executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return 'loading'; } })()");
    return readyState == "complete" || readyState == "interactive";
}

bool Browser::validateSession(const Session& session) const {
    if (session.getName().empty()) {
        std::cerr << "Warning: Session has empty name" << std::endl;
        return false;
    }
    
    const std::string& url = session.getCurrentUrl();
    if (!url.empty()) {
        if (url.find("://") == std::string::npos) {
            std::cerr << "Warning: Session URL appears invalid: " << url << std::endl;
            return false;
        }
    }
    
    return true;
}

std::string Browser::getPageLoadState() const {
    return const_cast<Browser*>(this)->executeJavascriptSync("(function() { try { return document.readyState + '|' + window.location.href; } catch(e) { return 'error|unknown'; } })()");
}

bool Browser::restoreSessionSafely(const Session& session) {
    if (!validateSession(session)) {
        std::cerr << "Warning: Session validation failed, continuing with limited restore" << std::endl;
    }
    
    try {
        restoreSession(session);
        
        waitForPageStabilization();
        
        if (!isPageLoaded()) {
            std::cerr << "Warning: Page may not have loaded completely after session restore" << std::endl;
            std::cerr << "Page state: " << getPageLoadState() << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error during session restore: " << e.what() << std::endl;
        return false;
    }
}

void Browser::waitForPageStabilization(int timeout_ms) {
    int elapsed = 0;
    std::string previousState = "";
    
    while (elapsed < timeout_ms) {
        std::string currentState = getPageLoadState();
        
        if (!previousState.empty() && currentState == previousState) {
            wait(200);
            return;
        }
        
        previousState = currentState;
        wait(200);
        elapsed += 200;
    }
    
    std::cerr << "Warning: Page stabilization timeout after " << timeout_ms << "ms" << std::endl;
}

std::string Browser::executeJavascriptSyncSafe(const std::string& script) {
    try {
        if (!webView) {
            std::cerr << "Error: WebView not initialized" << std::endl;
            return "";
        }
        
        if (script.empty()) {
            return "";
        }
        
        if (!isPageLoaded()) {
            std::cerr << "Warning: Executing JavaScript on potentially unready page" << std::endl;
        }
        
        return executeJavascriptSync(script);
    } catch (const std::exception& e) {
        std::cerr << "Error in JavaScript execution: " << e.what() << std::endl;
        return "";
    }
}

void Browser::setCookieSafe(const Cookie& cookie) {
    if (cookie.name.empty() || cookie.value.empty()) {
        std::cerr << "Warning: Skipping invalid cookie (empty name or value)" << std::endl;
        return;
    }
    
    try {
        setCookie(cookie);
        
        std::string verification = executeJavascriptSyncSafe(
            "(function() { "
            "  try { "
            "    var cookies = document.cookie.split(';'); "
            "    for (var i = 0; i < cookies.length; i++) { "
            "      if (cookies[i].trim().startsWith('" + cookie.name + "=')) { "
            "        return 'found'; "
            "      } "
            "    } "
            "    return 'not_found'; "
            "  } catch(e) { return 'error'; } "
            "})()"
        );
        
        if (verification != "found") {
            std::cerr << "Warning: Cookie '" << cookie.name << "' may not have been set properly" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error setting cookie '" << cookie.name << "': " << e.what() << std::endl;
    }
}

bool Browser::isFileUrl(const std::string& url) const {
    return url.find("file://") == 0;
}

bool Browser::validateFileUrl(const std::string& url) const {
    if (!isFileUrl(url)) {
        return true;
    }
    
    std::string filePath = url.substr(7);
    
    try {
        return std::filesystem::exists(filePath);
    } catch (const std::exception& e) {
        std::cerr << "Error validating file URL: " << e.what() << std::endl;
        return false;
    }
}
