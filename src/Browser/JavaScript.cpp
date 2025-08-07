#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <glib.h>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

// External debug flag
extern bool g_debug;

// Callback for JavaScript evaluation
void js_eval_callback(GObject* object, GAsyncResult* res, gpointer user_data) {
    // CRITICAL SAFETY: Validate all pointers before any operations
    if (!object || !res) {
        return;
    }
    
    GError* error = NULL;
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), res, &error);
    
    Browser* browser_instance = static_cast<Browser*>(g_object_get_data(G_OBJECT(object), "browser-instance"));
    
    // CRITICAL FIX: Check if browser object is still valid
    if (browser_instance && !browser_instance->isObjectValid()) {
        browser_instance = nullptr;
    }
    
    if (error) {
        // Don't log common errors that are expected in test environment
        bool should_suppress = strstr(error->message, "SecurityError") || 
                              strstr(error->message, "ReferenceError: Can't find variable") ||
                              strstr(error->message, "localStorage") ||
                              strstr(error->message, "sessionStorage") ||
                              strstr(error->message, "SyntaxError: Unexpected end of script");
        
        if (!should_suppress) {
            std::cerr << "JavaScript error: " << error->message << std::endl;
        }
        g_error_free(error);
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    } else if (value) {
        // DEBUG: Log when we have a valid value
        debug_output("JavaScript callback: Valid value received");
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
        // CRITICAL FIX: Handle case where JavaScript returns null/undefined but no error occurs
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
        // DEBUG: Log when we get neither error nor value
        debug_output("JavaScript callback: No error and no value - this is unusual");
    }
    
    if (browser_instance && browser_instance->event_loop_manager) {
        browser_instance->event_loop_manager->signalJavaScriptComplete();
    }
}

// ========== JavaScript Execution Methods ==========

void Browser::executeJavascript(const std::string& script, std::string* result) {
    // SAFETY FIX: Deprecate this unsafe method by routing to safe implementation
    // This maintains backward compatibility while eliminating memory corruption
    
    if (!result) {
        // If no result is needed, just execute without storing result
        executeJavascriptSyncSafe(script);
        return;
    }
    
    // Route to safe synchronous implementation
    try {
        *result = executeJavascriptSyncSafe(script);
    } catch (const std::exception& e) {
        std::cerr << "Error in JavaScript execution: " << e.what() << std::endl;
        *result = "";
    }
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
        
        // EVENT-DRIVEN APPROACH: Process pending events with timeout instead of blocking
        auto start_time = std::chrono::steady_clock::now();
        while (!data.timed_out && 
               std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < timeout_ms) {
            // Process pending events non-blocking
            while (g_main_context_pending(g_main_context_default())) {
                g_main_context_iteration(g_main_context_default(), FALSE);
            }
            
            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
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
        return "";
    }
    
    // MEMORY SAFETY FIX: Use local buffer to avoid concurrent access corruption
    try {
        // Use a local string buffer for this specific execution
        std::string local_result_buffer;
        
        // Use local buffer to avoid race conditions with member buffer
        webkit_web_view_evaluate_javascript(
            webView, 
            script.c_str(), 
            -1,
            NULL, 
            NULL, 
            NULL, 
            js_eval_callback, 
            &local_result_buffer
        );
        
        // Wait for completion with timeout
        if (!waitForJavaScriptCompletion(5000)) {
            debug_output("JavaScript execution timeout for: " + script.substr(0, 50) + "...");
            return "";
        }
        
        // Get result from our local buffer (no need to clear member buffer)
        std::string return_value = local_result_buffer;
        
        // DEBUG: Log JavaScript execution for debugging
        if (g_debug && script.find("clickElement") != std::string::npos) {
            debug_output("JS Debug - Script: " + script.substr(0, 100) + "...");
            debug_output("JS Debug - Result: '" + return_value + "'");
        }
        
        // Handle common problematic return values
        if (return_value == "undefined" || return_value == "null") {
            return "";
        }
        
        // Ensure we don't have a ridiculously long result
        if (return_value.length() > 100000) {
            return return_value.substr(0, 100000);
        }
        
        return return_value;
        
    } catch (const std::exception& e) {
        debug_output("Error in JavaScript execution: " + std::string(e.what()));
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
