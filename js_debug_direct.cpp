#include <iostream>
#include <fstream>
#include <string>
#include <webkit2/webkit2.h>
#include <gtk/gtk.h>

// Global variables for capturing JavaScript errors
std::string last_js_error = "";
int error_line = 0;

// JavaScript error handler
static void on_script_exception(WebKitWebView* web_view, WebKitJavascriptResult* js_result, gpointer user_data) {
    std::cout << "JavaScript execution completed" << std::endl;
}

static gboolean on_script_error(WebKitWebView* web_view, WebKitConsoleMessage* message, gpointer user_data) {
    const char* text = webkit_console_message_get_text(message);
    guint line = webkit_console_message_get_line(message);
    
    std::cout << "JavaScript error at line " << line << ": " << text << std::endl;
    
    if (line == 59) {
        last_js_error = text;
        error_line = line;
        
        // Try to capture the source document at the moment of error
        webkit_web_view_run_javascript(web_view, 
            "document.documentElement.outerHTML",
            NULL, on_script_exception, NULL);
    }
    
    return TRUE;
}

static void on_load_finished(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        std::cout << "Page load finished, trying to capture any injected JavaScript..." << std::endl;
        
        // Try to inspect all script elements
        webkit_web_view_run_javascript(web_view, 
            R"(
            var scripts = document.querySelectorAll('script');
            var result = 'Scripts found: ' + scripts.length + '\n';
            for (var i = 0; i < scripts.length; i++) {
                result += 'Script ' + i + ': ' + scripts[i].textContent.substring(0, 200) + '\n';
            }
            result;
            )", 
            NULL, on_script_exception, NULL);
    }
}

int main() {
    gtk_init(NULL, NULL);
    
    // Create a minimal webkit view
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    // Set up error capturing
    g_signal_connect(web_view, "console-message", G_CALLBACK(on_script_error), NULL);
    g_signal_connect(web_view, "load-changed", G_CALLBACK(on_load_finished), NULL);
    
    // Load our test HTML file
    std::string file_url = "file:///home/bigattichouse/workspace/headlessweb/tests/sample_html/minimal_input_test.html";
    webkit_web_view_load_uri(web_view, file_url.c_str());
    
    std::cout << "Loading: " << file_url << std::endl;
    std::cout << "Waiting for JavaScript errors at line 59..." << std::endl;
    
    // Run GTK main loop briefly
    for (int i = 0; i < 100; i++) {
        gtk_main_iteration_do(FALSE);
        usleep(50000); // 50ms
        
        if (error_line == 59) {
            std::cout << "Captured line 59 error: " << last_js_error << std::endl;
            break;
        }
    }
    
    return 0;
}