#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <glib.h>
#include <iostream>
#include <cmath>

// External debug flag
extern bool g_debug;

// Callback for JavaScript evaluation
void js_eval_callback(GObject* object, GAsyncResult* res, gpointer user_data) {
    GError* error = NULL;
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), res, &error);
    
    Browser* browser_instance = static_cast<Browser*>(g_object_get_data(G_OBJECT(object), "browser-instance"));
    
    if (error) {
        // Don't log SecurityError for localStorage/sessionStorage on file:// URLs - it's expected
        if (!strstr(error->message, "SecurityError")) {
            std::cerr << "JavaScript error: " << error->message << std::endl;
        }
        g_error_free(error);
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    } else if (value) {
        if (user_data) {
            try {
                if (jsc_value_is_string(value)) {
                    char* str_value = jsc_value_to_string(value);
                    if (str_value) {
                        *(static_cast<std::string*>(user_data)) = str_value;
                        g_free(str_value);
                    } else {
                        *(static_cast<std::string*>(user_data)) = "";
                    }
                } else if (jsc_value_is_number(value)) {
                    double num_val = jsc_value_to_double(value);
                    if (num_val == floor(num_val)) {
                        *(static_cast<std::string*>(user_data)) = std::to_string((long long)num_val);
                    } else {
                        *(static_cast<std::string*>(user_data)) = std::to_string(num_val);
                    }
                } else if (jsc_value_is_boolean(value)) {
                    *(static_cast<std::string*>(user_data)) = jsc_value_to_boolean(value) ? "true" : "false";
                } else if (jsc_value_is_null(value)) {
                    *(static_cast<std::string*>(user_data)) = "null";
                } else if (jsc_value_is_undefined(value)) {
                    *(static_cast<std::string*>(user_data)) = "undefined";
                } else if (jsc_value_is_object(value)) {
                    // Try to convert object to string
                    char* str_value = jsc_value_to_string(value);
                    if (str_value) {
                        *(static_cast<std::string*>(user_data)) = str_value;
                        g_free(str_value);
                    } else {
                        *(static_cast<std::string*>(user_data)) = "[object Object]";
                    }
                } else {
                    // Fallback: try to convert to string
                    char* str_value = jsc_value_to_string(value);
                    if (str_value) {
                        *(static_cast<std::string*>(user_data)) = str_value;
                        g_free(str_value);
                    } else {
                        *(static_cast<std::string*>(user_data)) = "";
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing JavaScript result: " << e.what() << std::endl;
                *(static_cast<std::string*>(user_data)) = "";
            }
        }
        g_object_unref(value);
    } else {
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    }
    
    if (browser_instance && browser_instance->event_loop_manager) {
        browser_instance->event_loop_manager->signalJavaScriptComplete();
    }
}

// ========== JavaScript Execution Methods ==========

void Browser::executeJavascript(const std::string& script, std::string* result) {
    if (result) {
        result->clear();
    }
    
    if (script.empty()) {
        std::cerr << "Warning: Empty JavaScript script" << std::endl;
        if (result) {
            *result = "";
        }
        return;
    }
    
    if (!webView) {
        std::cerr << "Error: WebView is null" << std::endl;
        if (result) {
            *result = "";
        }
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
    if (!event_loop_manager) {
        debug_output("EventLoopManager not initialized, falling back to direct wait");
        // Fallback to old method
        struct CompletionData {
            GMainLoop* loop;
            bool timed_out;
            guint timeout_id;
        };
        
        CompletionData data = {main_loop, false, 0};
        
        data.timeout_id = g_timeout_add(timeout_ms, [](gpointer user_data) -> gboolean {
            CompletionData* data = static_cast<CompletionData*>(user_data);
            data->timed_out = true;
            if (g_main_loop_is_running(data->loop)) {
                g_main_loop_quit(data->loop);
            }
            return G_SOURCE_REMOVE;
        }, &data);
        
        g_main_loop_run(main_loop);
        
        if (!data.timed_out && data.timeout_id != 0) {
            g_source_remove(data.timeout_id);
        }
        
        return !data.timed_out;
    }
    
    return event_loop_manager->waitForJavaScriptCompletion(timeout_ms);
}

std::string Browser::executeJavascriptSync(const std::string& script) {
    if (script.empty() || !webView) {
        return "";
    }
    
    // Check if we have a valid document context
    const gchar* uri = webkit_web_view_get_uri(webView);
    if (!uri || strlen(uri) == 0) {
        debug_output("No URI loaded, JavaScript execution may hang. Script: " + script.substr(0, 50) + "...");
        // For now, return empty to prevent hangs
        return "";
    }
    
    js_result_buffer.clear();
    
    try {
        executeJavascript(script, &js_result_buffer);
        if (!waitForJavaScriptCompletion(5000)) {
            debug_output("JavaScript execution timeout for: " + script.substr(0, 50) + "...");
            return "";
        }
        
        // Clean up the result buffer
        std::string result = js_result_buffer;
        
        // Handle common problematic return values
        if (result.empty() || result == "undefined" || result == "null") {
            return "";
        }
        
        // Ensure we don't have a ridiculously long result
        if (result.length() > 100000) {
            return result.substr(0, 100000);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in JavaScript execution: " << e.what() << std::endl;
        return "";
    }
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
