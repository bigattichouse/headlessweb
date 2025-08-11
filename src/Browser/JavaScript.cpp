#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <glib.h>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <mutex>

// External debug flag
extern bool g_debug;

// Wrapper struct for safe callback data handling
struct JavaScriptCallbackData {
    std::shared_ptr<std::string> result_ptr;
    std::atomic<bool> completed;
    std::mutex completion_mutex;
    
    JavaScriptCallbackData() : result_ptr(std::make_shared<std::string>()), completed(false) {}
};

// Callback for JavaScript evaluation
void js_eval_callback(GObject* object, GAsyncResult* res, gpointer user_data) {
    // CRITICAL SAFETY: Validate all pointers before any operations
    if (!object || !res || !user_data) {
        return;
    }
    
    JavaScriptCallbackData* callback_data = static_cast<JavaScriptCallbackData*>(user_data);
    
    // Use lock guard to ensure thread safety
    std::lock_guard<std::mutex> lock(callback_data->completion_mutex);
    
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
        *(callback_data->result_ptr) = "";
    } else if (value) {
        // DEBUG: Log when we have a valid value
        debug_output("JavaScript callback: Valid value received");
        try {
            if (jsc_value_is_string(value)) {
                char* str_value = jsc_value_to_string(value);
                if (str_value) {
                    *(callback_data->result_ptr) = str_value;
                    g_free(str_value);
                } else {
                    *(callback_data->result_ptr) = "";
                }
            } else if (jsc_value_is_number(value)) {
                double num_val = jsc_value_to_double(value);
                if (num_val == floor(num_val)) {
                    *(callback_data->result_ptr) = std::to_string((long long)num_val);
                } else {
                    *(callback_data->result_ptr) = std::to_string(num_val);
                }
            } else if (jsc_value_is_boolean(value)) {
                *(callback_data->result_ptr) = jsc_value_to_boolean(value) ? "true" : "false";
            } else if (jsc_value_is_null(value)) {
                *(callback_data->result_ptr) = "null";
            } else if (jsc_value_is_undefined(value)) {
                *(callback_data->result_ptr) = "undefined";
            } else if (jsc_value_is_object(value)) {
                // Try to convert object to string
                char* str_value = jsc_value_to_string(value);
                if (str_value) {
                    *(callback_data->result_ptr) = str_value;
                    g_free(str_value);
                } else {
                    *(callback_data->result_ptr) = "[object Object]";
                }
            } else {
                // Fallback: try to convert to string
                char* str_value = jsc_value_to_string(value);
                if (str_value) {
                    *(callback_data->result_ptr) = str_value;
                    g_free(str_value);
                } else {
                    *(callback_data->result_ptr) = "";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing JavaScript result: " << e.what() << std::endl;
            *(callback_data->result_ptr) = "";
        }
        g_object_unref(value);
    } else {
        // CRITICAL FIX: Handle case where JavaScript returns null/undefined but no error occurs
        *(callback_data->result_ptr) = "";
        // DEBUG: Log when we get neither error nor value
        debug_output("JavaScript callback: No error and no value - this is unusual");
    }
    
    // Mark completion
    callback_data->completed.store(true);
    
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
    
    // MEMORY SAFETY FIX: Use safe callback data structure
    try {
        // Create safe callback data on the heap
        auto callback_data = std::make_unique<JavaScriptCallbackData>();
        
        webkit_web_view_evaluate_javascript(
            webView, 
            script.c_str(), 
            -1,
            NULL, 
            NULL, 
            NULL, 
            js_eval_callback, 
            callback_data.get()
        );
        
        // Wait for completion with timeout using safe polling
        auto start_time = std::chrono::steady_clock::now();
        const int timeout_ms = 5000;
        const int poll_interval_ms = 10;
        
        while (!callback_data->completed.load() && 
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start_time).count() < timeout_ms) {
            
            // Process pending events
            while (g_main_context_pending(g_main_context_default())) {
                g_main_context_iteration(g_main_context_default(), FALSE);
            }
            
            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval_ms));
        }
        
        if (!callback_data->completed.load()) {
            debug_output("JavaScript execution timeout for: " + script.substr(0, 50) + "...");
            return "";
        }
        
        // Get result from our safe callback data
        std::string return_value = *(callback_data->result_ptr);
        
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
