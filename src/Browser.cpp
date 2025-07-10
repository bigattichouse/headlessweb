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
    
    if (error) {
        std::cerr << "JavaScript error: " << error->message << std::endl;
        g_error_free(error);
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    } else if (value && user_data) {
        if (jsc_value_is_string(value)) {
            char* str_value = jsc_value_to_string(value);
            *(static_cast<std::string*>(user_data)) = str_value;
            g_free(str_value);
        } else if (jsc_value_is_number(value)) {
            *(static_cast<std::string*>(user_data)) = std::to_string(jsc_value_to_double(value));
        } else if (jsc_value_is_boolean(value)) {
            *(static_cast<std::string*>(user_data)) = jsc_value_to_boolean(value) ? "true" : "false";
        } else if (jsc_value_is_null(value)) {
            *(static_cast<std::string*>(user_data)) = "null";
        } else if (jsc_value_is_undefined(value)) {
            *(static_cast<std::string*>(user_data)) = "undefined";
        } else {
            *(static_cast<std::string*>(user_data)) = "[JS Result]";
        }
    }
    
    Browser* browser_instance = static_cast<Browser*>(g_object_get_data(G_OBJECT(object), "browser-instance"));
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

Browser::Browser() {
    gtk_init();
    
    // Initialize with a proper data manager for cookie/storage persistence
    std::string home = std::getenv("HOME");
    sessionDataPath = home + "/.hweb-poc/webkit-data";
    std::filesystem::create_directories(sessionDataPath);
    
    // In WebKitGTK+ 6.0, we create the web view directly with settings
    // The data manager is handled differently
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
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
    operation_completed = false;
    g_object_set_data(G_OBJECT(webView), "browser-instance", this);
    
    g_object_unref(settings);
}

Browser::~Browser() {
    // Cleanup handled by GTK
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
    webkit_web_view_evaluate_javascript(webView, script.c_str(), -1, NULL, NULL, NULL, js_eval_callback, result ? static_cast<gpointer>(result) : NULL);
}

bool Browser::waitForJavaScriptCompletion(int timeout_ms) {
    int elapsed_time = 0;
    while (!isOperationCompleted() && elapsed_time < timeout_ms) {
        g_main_context_iteration(g_main_context_default(), FALSE);
        g_usleep(10 * 1000); // Sleep for 10ms
        elapsed_time += 10;
    }
    return operation_completed;
}

void Browser::restoreSession(const Session& session) {
    // Set user agent
    if (!session.getUserAgent().empty()) {
        setUserAgent(session.getUserAgent());
    }
    
    // Navigate to current URL
    if (!session.getCurrentUrl().empty()) {
        loadUri(session.getCurrentUrl());
        waitForJavaScriptCompletion(10000);
    }
    
    // Wait for page to be ready based on conditions
    waitForPageReady(session);
    
    // Restore cookies
    for (const auto& cookie : session.getCookies()) {
        setCookie(cookie);
    }
    
    // Restore local storage
    if (!session.getLocalStorage().empty()) {
        setLocalStorage(session.getLocalStorage());
    }
    
    // Restore session storage
    if (!session.getSessionStorage().empty()) {
        setSessionStorage(session.getSessionStorage());
    }
    
    // Restore form state
    if (!session.getFormFields().empty()) {
        restoreFormState(session.getFormFields());
    }
    
    // Restore active elements
    if (!session.getActiveElements().empty()) {
        restoreActiveElements(session.getActiveElements());
    }
    
    // Restore custom state
    if (!session.getAllExtractedState().empty()) {
        restoreCustomState(session.getAllExtractedState());
    }
    
    // Restore viewport
    auto [width, height] = session.getViewport();
    setViewport(width, height);
    
    // Restore scroll positions
    if (!session.getAllScrollPositions().empty()) {
        restoreScrollPositions(session.getAllScrollPositions());
    }
}

void Browser::updateSessionState(Session& session) {
    // Update current URL
    session.setCurrentUrl(getCurrentUrl());
    
    // Update page hash
    session.setPageHash(extractPageHash());
    
    // Update document ready state
    session.setDocumentReadyState(extractDocumentReadyState());
    
    // Get cookies asynchronously
    getCookiesAsync([&session](std::vector<Cookie> cookies) {
        session.setCookies(cookies);
    });
    
    // Wait for cookie operation to complete
    waitForJavaScriptCompletion(2000);
    
    // Update storage
    session.setLocalStorage(getLocalStorage());
    session.setSessionStorage(getSessionStorage());
    
    // Extract form state
    session.setFormFields(extractFormState());
    
    // Extract active elements
    session.setActiveElements(extractActiveElements());
    
    // Extract scroll positions
    auto scrollPositions = extractAllScrollPositions();
    for (const auto& [selector, pos] : scrollPositions) {
        session.setScrollPosition(selector, pos.first, pos.second);
    }
    
    // Extract custom state using registered extractors
    if (!session.getStateExtractors().empty()) {
        auto customState = extractCustomState(session.getStateExtractors());
        
        // Fixed: iterate over member names properly
        auto memberNames = customState.getMemberNames();
        for (const auto& key : memberNames) {
            session.setExtractedState(key, customState[key]);
        }
    }
    
    // Update last accessed time
    session.updateLastAccessed();
}

// Fixed lambda function for cookies callback
struct CookieCallbackData {
    std::function<void(std::vector<Cookie>)> callback;
    std::string* result;
    Browser* browser;
};

void Browser::getCookiesAsync(std::function<void(std::vector<Cookie>)> callback) {
    std::string js = R"(
        (function() {
            var cookies = document.cookie.split('; ');
            var result = [];
            for (var i = 0; i < cookies.length; i++) {
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
            return JSON.stringify(result);
        })()
    )";
    
    auto* data = new CookieCallbackData{callback, new std::string(), this};
    
    operation_completed = false;
    webkit_web_view_evaluate_javascript(
        webView, 
        js.c_str(), 
        -1, 
        NULL, 
        NULL, 
        NULL, 
        [](GObject* object, GAsyncResult* res, gpointer user_data) {
            auto* data = static_cast<CookieCallbackData*>(user_data);
            GError* error = NULL;
            JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), res, &error);
            
            std::vector<Cookie> cookies;
            
            if (!error && value && jsc_value_is_string(value)) {
                char* str_value = jsc_value_to_string(value);
                
                // Parse JSON result
                Json::Value root;
                Json::Reader reader;
                if (reader.parse(str_value, root) && root.isArray()) {
                    for (const auto& cookieJson : root) {
                        Cookie cookie;
                        cookie.name = cookieJson.get("name", "").asString();
                        cookie.value = cookieJson.get("value", "").asString();
                        cookie.domain = cookieJson.get("domain", "").asString();
                        cookie.path = cookieJson.get("path", "/").asString();
                        cookie.secure = false;
                        cookie.httpOnly = false;
                        cookie.expires = -1; // Session cookie
                        cookies.push_back(cookie);
                    }
                }
                
                g_free(str_value);
            }
            
            if (error) {
                g_error_free(error);
            }
            
            data->callback(cookies);
            delete data->result;
            
            if (data->browser) {
                data->browser->operation_completed = true;
            }
            
            delete data;
        },
        data
    );
}

void Browser::setCookie(const Cookie& cookie) {
    // Use JavaScript to set cookies
    std::stringstream js;
    js << "document.cookie = \"" << cookie.name << "=" << cookie.value;
    
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
    
    js << "\";";
    
    executeJavascript(js.str());
    waitForJavaScriptCompletion(500);
}

std::map<std::string, std::string> Browser::getLocalStorage() {
    std::string js = R"(
        (function() {
            var result = {};
            for (var i = 0; i < localStorage.length; i++) {
                var key = localStorage.key(i);
                result[key] = localStorage.getItem(key);
            }
            return JSON.stringify(result);
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
    executeJavascript("localStorage.clear();");
    waitForJavaScriptCompletion(500);
    
    for (const auto& [key, value] : storage) {
        std::stringstream js;
        js << "localStorage.setItem('" << key << "', '" << value << "');";
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::map<std::string, std::string> Browser::getSessionStorage() {
    std::string js = R"(
        (function() {
            var result = {};
            for (var i = 0; i < sessionStorage.length; i++) {
                var key = sessionStorage.key(i);
                result[key] = sessionStorage.getItem(key);
            }
            return JSON.stringify(result);
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
    executeJavascript("sessionStorage.clear();");
    waitForJavaScriptCompletion(500);
    
    for (const auto& [key, value] : storage) {
        std::stringstream js;
        js << "sessionStorage.setItem('" << key << "', '" << value << "');";
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::vector<FormField> Browser::extractFormState() {
    std::string js = R"(
        (function() {
            var fields = [];
            var elements = document.querySelectorAll('input, textarea, select');
            
            elements.forEach(function(el, index) {
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
                    field.selector = el.tagName.toLowerCase() + ':nth-of-type(' + (index + 1) + ')';
                }
                
                // Get value based on type
                if (el.type === 'checkbox' || el.type === 'radio') {
                    field.checked = el.checked;
                    field.value = el.value;
                } else if (el.tagName === 'SELECT') {
                    field.value = el.value;
                } else {
                    field.value = el.value;
                }
                
                fields.push(field);
            });
            
            return JSON.stringify(fields);
        })()
    )";
    
    std::string result = executeJavascriptSync(js);
    std::vector<FormField> fields;
    
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
    
    return fields;
}

void Browser::restoreFormState(const std::vector<FormField>& fields) {
    for (const auto& field : fields) {
        std::stringstream js;
        
        if (field.type == "checkbox" || field.type == "radio") {
            js << "var el = document.querySelector('" << field.selector << "'); ";
            js << "if (el) { el.checked = " << (field.checked ? "true" : "false") << "; ";
            js << "el.dispatchEvent(new Event('change', { bubbles: true })); }";
        } else {
            js << "var el = document.querySelector('" << field.selector << "'); ";
            js << "if (el) { el.value = '" << field.value << "'; ";
            js << "el.dispatchEvent(new Event('input', { bubbles: true })); ";
            js << "el.dispatchEvent(new Event('change', { bubbles: true })); }";
        }
        
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::set<std::string> Browser::extractActiveElements() {
    std::string js = R"(
        (function() {
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
            
            // Elements with aria-selected="true"
            document.querySelectorAll('[aria-selected="true"]').forEach(function(el) {
                if (el.id) {
                    active.push('#' + el.id);
                }
            });
            
            // Active tabs, accordions, etc.
            document.querySelectorAll('.active, .selected, .open').forEach(function(el) {
                if (el.id) {
                    active.push('#' + el.id);
                }
            });
            
            return JSON.stringify(active);
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
        js << "var el = document.querySelector('" << selector << "'); ";
        js << "if (el) { ";
        js << "  if (el.type === 'checkbox' || el.type === 'radio') { el.checked = true; } ";
        js << "  else if (el.tagName === 'OPTION') { el.selected = true; } ";
        js << "  else { el.classList.add('active'); } ";
        js << "  el.dispatchEvent(new Event('change', { bubbles: true })); ";
        js << "}";
        
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(1000);
}

std::string Browser::extractPageHash() {
    return executeJavascriptSync("window.location.hash");
}

std::string Browser::extractDocumentReadyState() {
    return executeJavascriptSync("document.readyState");
}

std::map<std::string, std::pair<int, int>> Browser::extractAllScrollPositions() {
    std::string js = R"(
        (function() {
            var positions = {};
            
            // Window scroll
            positions['window'] = [window.pageXOffset, window.pageYOffset];
            
            // Find all scrollable elements
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
                        positions[selector] = [el.scrollLeft, el.scrollTop];
                    }
                }
            }
            
            return JSON.stringify(positions);
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
            js << "window.scrollTo(" << pos.first << ", " << pos.second << ");";
        } else {
            js << "var el = document.querySelector('" << selector << "'); ";
            js << "if (el) { el.scrollLeft = " << pos.first << "; el.scrollTop = " << pos.second << "; }";
        }
        
        executeJavascript(js.str());
    }
    waitForJavaScriptCompletion(500);
}

bool Browser::waitForPageReady(const Session& session) {
    // First wait for basic document ready
    waitForJsCondition("document.readyState === 'complete'", 10000);
    
    // Then wait for any custom ready conditions
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
                // Execute custom ready check
                executeJavascript(condition.value);
                waitForJavaScriptCompletion(condition.timeout);
                break;
        }
    }
    
    // Additional wait for dynamic content to settle
    wait(500);
    
    return true;
}

bool Browser::waitForJsCondition(const std::string& condition, int timeout_ms) {
    int elapsed_time = 0;
    std::string js_check = "(function() { return " + condition + "; })()";
    
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
        
        // Try to parse as JSON
        Json::Value parsed;
        Json::Reader reader;
        if (reader.parse(extractedValue, parsed)) {
            result[name] = parsed;
        } else {
            // Store as string if not valid JSON
            result[name] = extractedValue;
        }
    }
    
    return result;
}

void Browser::restoreCustomState(const std::map<std::string, Json::Value>& state) {
    // This is application-specific and would need custom restore code
    // For now, we'll just log what would be restored
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
    js << "  var select = document.querySelector('" << selector << "'); ";
    js << "  if (select) { ";
    js << "    select.value = '" << value << "'; ";
    js << "    select.dispatchEvent(new Event('change', { bubbles: true })); ";
    js << "    return true; ";
    js << "  } ";
    js << "  return false; ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::checkElement(const std::string& selector) {
    std::stringstream js;
    js << "(function() { ";
    js << "  var el = document.querySelector('" << selector << "'); ";
    js << "  if (el && (el.type === 'checkbox' || el.type === 'radio')) { ";
    js << "    el.checked = true; ";
    js << "    el.dispatchEvent(new Event('change', { bubbles: true })); ";
    js << "    return true; ";
    js << "  } ";
    js << "  return false; ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::uncheckElement(const std::string& selector) {
    std::stringstream js;
    js << "(function() { ";
    js << "  var el = document.querySelector('" << selector << "'); ";
    js << "  if (el && (el.type === 'checkbox' || el.type === 'radio')) { ";
    js << "    el.checked = false; ";
    js << "    el.dispatchEvent(new Event('change', { bubbles: true })); ";
    js << "    return true; ";
    js << "  } ";
    js << "  return false; ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::focusElement(const std::string& selector) {
    std::stringstream js;
    js << "(function() { ";
    js << "  var el = document.querySelector('" << selector << "'); ";
    js << "  if (el) { ";
    js << "    el.focus(); ";
    js << "    return true; ";
    js << "  } ";
    js << "  return false; ";
    js << "})()";
    
    std::string result = executeJavascriptSync(js.str());
    return result == "true";
}

bool Browser::elementExists(const std::string& selector) {
    std::string js = "document.querySelector('" + selector + "') !== null";
    std::string result = executeJavascriptSync(js);
    return result == "true";
}

int Browser::countElements(const std::string& selector) {
    std::string js = "document.querySelectorAll('" + selector + "').length";
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
    js << "  var el = document.querySelector('" << selector << "'); ";
    js << "  return el ? el.outerHTML : ''; ";
    js << "})()";
    
    return executeJavascriptSync(js.str());
}

void Browser::takeScreenshot(const std::string& filename) {
    // WebKitGTK+ 6.0 screenshot implementation would go here
    std::cout << "Screenshot functionality not implemented in this version" << std::endl;
}

std::string Browser::getPageSource() {
    return executeJavascriptSync("document.documentElement.outerHTML");
}

// Existing methods remain the same...

std::string Browser::executeJavascriptSync(const std::string& script) {
    std::string result;
    executeJavascript(script, &result);
    waitForJavaScriptCompletion();
    return result;
}

void Browser::wait(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

bool Browser::isOperationCompleted() const {
    return operation_completed;
}

bool Browser::waitForSelector(const std::string& selector, int timeout_ms) {
    int elapsed_time = 0;
    std::string js_check_script = "document.querySelector('" + selector + "') != null;";
    std::string js_result_str;

    while (elapsed_time < timeout_ms) {
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

    std::string js_check_script = "document.body.innerText.includes('" + escaped_text + "');";
    
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
        "  var element = document.querySelector('" + selector + "'); "
        "  return element && (element.innerText.trim() || element.textContent.trim()) ? true : false; "
        "})()";
    
    return waitForJsCondition(js_check, timeout_ms);
}

std::string Browser::getInnerText(const std::string& selector) {
    std::string text_content_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  return element ? (element.innerText || element.textContent || '') : ''; "
        "})()";
    
    return executeJavascriptSync(text_content_script);
}

std::string Browser::getFirstNonEmptyText(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  var elements = document.querySelectorAll('" + selector + "'); "
        "  for (var i = 0; i < elements.length; i++) { "
        "    var text = elements[i].innerText || elements[i].textContent || ''; "
        "    if (text.trim()) { "
        "      return text.trim(); "
        "    } "
        "  } "
        "  return ''; "
        "})()";
    
    return executeJavascriptSync(js_script);
}

bool Browser::fillInput(const std::string& selector, const std::string& value) {
    std::string js_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  if (element) { "
        "    element.value = '" + value + "'; "
        "    element.dispatchEvent(new Event('input', { bubbles: true })); "
        "    element.dispatchEvent(new Event('change', { bubbles: true })); "
        "    return true; "
        "  } "
        "  return false; "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::clickElement(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  if (element) { "
        "    element.click(); "
        "    return true; "
        "  } "
        "  return false; "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::submitForm(const std::string& form_selector) {
    std::string js_script = 
        "(function() { "
        "  var form = document.querySelector('" + form_selector + "'); "
        "  if (form) { "
        "    form.submit(); "
        "    return true; "
        "  } "
        "  return false; "
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
            // Wait a bit more for the page to stabilize
            wait(500);
            return true;
        }
    }
    
    return false;
}

bool Browser::searchForm(const std::string& query) {
    std::string js_script = 
        "(function() { "
        "  var searchInputs = document.querySelectorAll('input[name*=search], input[type=search], input[placeholder*=search i], input[placeholder*=Search]'); "
        "  var searchButtons = document.querySelectorAll('button[name*=search], input[type=submit], button[type=submit]'); "
        "  "
        "  if (searchInputs.length > 0) { "
        "    searchInputs[0].value = '" + query + "'; "
        "    searchInputs[0].dispatchEvent(new Event('input', { bubbles: true })); "
        "    searchInputs[0].dispatchEvent(new Event('change', { bubbles: true })); "
        "    "
        "    if (searchButtons.length > 0) { "
        "      searchButtons[0].click(); "
        "    } else { "
        "      searchInputs[0].form && searchInputs[0].form.submit(); "
        "    } "
        "    return true; "
        "  } "
        "  return false; "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

std::string Browser::getAttribute(const std::string& selector, const std::string& attribute) {
    std::string js_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  return element ? (element.getAttribute('" + attribute + "') || '') : ''; "
        "})()";
    
    return executeJavascriptSync(js_script);
}

void Browser::setViewport(int width, int height) {
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
}

void Browser::setScrollPosition(int x, int y) {
    std::stringstream js;
    js << "window.scrollTo(" << x << ", " << y << ");";
    executeJavascript(js.str());
    waitForJavaScriptCompletion(500);
}

std::pair<int, int> Browser::getScrollPosition() {
    std::string js = "JSON.stringify({x: window.pageXOffset, y: window.pageYOffset});";
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
    // In WebKitGTK+ 6.0, we clear cookies using JavaScript
    std::string js = R"(
        (function() {
            // Get all cookies
            var cookies = document.cookie.split(';');
            
            // Clear each cookie
            for (var i = 0; i < cookies.length; i++) {
                var cookie = cookies[i];
                var eqPos = cookie.indexOf("=");
                var name = eqPos > -1 ? cookie.substr(0, eqPos).trim() : cookie.trim();
                
                // Delete cookie by setting expiration date to past
                document.cookie = name + "=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/";
                document.cookie = name + "=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=" + window.location.hostname;
                document.cookie = name + "=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=." + window.location.hostname;
            }
            
            return 'Cookies cleared';
        })()
    )";
    
    executeJavascript(js);
    waitForJavaScriptCompletion(500);
}
