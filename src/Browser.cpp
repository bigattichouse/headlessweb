#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#include <libsoup/soup.h>
#include <unistd.h> // For sleep
#include <glib.h> // For g_main_context_iteration
#include <iostream> // For std::cout and std::endl

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

// Callback for getting cookies (simplified, no actual cookie retrieval)
static void get_cookies_callback(GObject* object, GAsyncResult* result, gpointer user_data) {
    // Removed cookie management for WebKitGTK+ 6.0 POC
    Browser* browser_instance = static_cast<Browser*>(g_object_get_data(object, "browser-instance"));
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
    
    // Create web view first
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    // Create and apply settings to disable audio/video
    WebKitSettings* settings = webkit_settings_new();
    webkit_settings_set_enable_media(settings, FALSE);
    webkit_settings_set_enable_media_stream(settings, FALSE);
    webkit_settings_set_enable_webaudio(settings, FALSE);
    
    // Apply settings to the web view
    webkit_web_view_set_settings(webView, settings);
    
    window = gtk_window_new();
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(webView));
    operation_completed = false;
    g_object_set_data(G_OBJECT(webView), "browser-instance", this);
    
    // Clean up settings reference
    g_object_unref(settings);
}

Browser::~Browser() {
    // No explicit GMainLoop to unref here
}

void Browser::loadUri(const std::string& uri) {
    operation_completed = false;
    g_signal_connect(webView, "load-changed", G_CALLBACK(load_changed_callback), this);
    webkit_web_view_load_uri(webView, uri.c_str());
}

void Browser::executeJavascript(const std::string& script, std::string* result) {
    operation_completed = false;
    webkit_web_view_evaluate_javascript(webView, script.c_str(), -1, NULL, NULL, NULL, js_eval_callback, result ? static_cast<gpointer>(result) : NULL);
}

std::string Browser::getCookies() {
    // Removed cookie management for WebKitGTK+ 6.0 POC
    return "";
}

void Browser::setCookies(const std::string& cookies) {
    // Removed cookie management for WebKitGTK+ 6.0 POC
}

bool Browser::isOperationCompleted() const {
    return operation_completed;
}

void Browser::waitForJavaScriptCompletion(int timeout_ms) {
    int elapsed_time = 0;
    while (!isOperationCompleted() && elapsed_time < timeout_ms) {
        g_main_context_iteration(g_main_context_default(), FALSE);
        g_usleep(10 * 1000); // Sleep for 10ms
        elapsed_time += 10;
    }
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
        pos = escaped_text.find("'");
    }

    std::string js_check_script = "document.body.innerText.includes('" + escaped_text + "');";
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

bool Browser::waitForElementWithContent(const std::string& selector, int timeout_ms) {
    int elapsed_time = 0;
    std::string js_check_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  return element && (element.innerText.trim() || element.textContent.trim()); "
        "})()";
    
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
        g_usleep(500 * 1000);
    }
    return false;
}

std::string Browser::getInnerText(const std::string& selector) {
    std::string text_content_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  return element ? (element.innerText || element.textContent || '') : ''; "
        "})()";
    
    std::string result = "";
    executeJavascript(text_content_script, &result);
    waitForJavaScriptCompletion();
    
    return result;
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
    
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    
    return result;
}

// Enhanced form operations
bool Browser::fillInput(const std::string& selector, const std::string& value) {
    std::string js_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  if (element) { "
        "    element.value = '" + value + "'; "
        "    element.dispatchEvent(new Event('input', { bubbles: true })); "
        "    return true; "
        "  } "
        "  return false; "
        "})()";
    
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    
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
    
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    
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
    
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    
    return result == "true";
}

bool Browser::waitForNavigation(int timeout_ms) {
    std::string initial_url = getCurrentUrl();
    int elapsed_time = 0;
    
    while (elapsed_time < timeout_ms) {
        g_usleep(500 * 1000); // Wait 500ms
        elapsed_time += 500;
        
        std::string current_url = getCurrentUrl();
        if (current_url != initial_url && !current_url.empty()) {
            return true;
        }
    }
    
    return false;
}

bool Browser::searchForm(const std::string& query) {
    // Try to find and fill a search form automatically
    std::string js_script = 
        "(function() { "
        "  var searchInputs = document.querySelectorAll('input[name*=search], input[type=search], input[placeholder*=search], input[placeholder*=Search]'); "
        "  var searchButtons = document.querySelectorAll('button[name*=search], input[type=submit], button[type=submit]'); "
        "  "
        "  if (searchInputs.length > 0 && searchButtons.length > 0) { "
        "    searchInputs[0].value = '" + query + "'; "
        "    searchInputs[0].dispatchEvent(new Event('input', { bubbles: true })); "
        "    searchButtons[0].click(); "
        "    return true; "
        "  } "
        "  return false; "
        "})()";
    
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    
    return result == "true";
}

std::string Browser::getCurrentUrl() {
    std::string js_script = "window.location.href;";
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    return result;
}

std::string Browser::getPageTitle() {
    std::string js_script = "document.title;";
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    return result;
}

std::string Browser::getAttribute(const std::string& selector, const std::string& attribute) {
    std::string js_script = 
        "(function() { "
        "  var element = document.querySelector('" + selector + "'); "
        "  return element ? (element.getAttribute('" + attribute + "') || '') : ''; "
        "})()";
    
    std::string result = "";
    executeJavascript(js_script, &result);
    waitForJavaScriptCompletion();
    
    return result;
}
