#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include <unistd.h> // For sleep

// Callback for JavaScript evaluation
static void js_eval_callback(GObject* object, GAsyncResult* res, gpointer user_data) {
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), res, NULL);
    if (value) {
        if (jsc_value_is_string(value)) {
            *(static_cast<std::string*>(user_data)) = jsc_value_to_string(value);
        } else if (jsc_value_is_number(value)) {
            *(static_cast<std::string*>(user_data)) = std::to_string(jsc_value_to_double(value));
        } else if (jsc_value_is_boolean(value)) {
            *(static_cast<std::string*>(user_data)) = jsc_value_to_boolean(value) ? "true" : "false";
        } else if (jsc_value_is_null(value)) {
            *(static_cast<std::string*>(user_data)) = "null";
        } else if (jsc_value_is_undefined(value)) {
            *(static_cast<std::string*>(user_data)) = "undefined";
        } else {
            *(static_cast<std::string*>(user_data)) = "[JS Result]"; // Generic for other types
        }
        // jsc_value_unref(value); // Removed: JSCValue might be managed by context
    }
    Browser* browser_instance = static_cast<Browser*>(g_object_get_data(object, "browser-instance"));
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
    gtk_init(); // GTK 4: no arguments
    window = gtk_window_new(); // GTK 4: no arguments, creates top-level by default
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(webView)); // GTK 4: new way to add child
    operation_completed = false;
    g_object_set_data(G_OBJECT(webView), "browser-instance", this); // Store browser instance
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
