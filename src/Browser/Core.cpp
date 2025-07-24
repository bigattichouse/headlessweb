#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cctype>

// External debug flag
extern bool g_debug;

// ========== Navigation Methods ==========

void Browser::loadUri(const std::string& uri) {
    // Use our comprehensive URL validation
    if (!validateUrl(uri)) {
        throw std::invalid_argument("Error: Invalid or unsafe URL: " + uri);
    }
    
    // Store current URL before navigating for proper waitForNavigation detection
    previous_url = getCurrentUrl();
    
    debug_output("Loading URI: " + uri + " (from: " + previous_url + ")");
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

// ========== URL Validation Methods ==========

bool Browser::isFileUrl(const std::string& url) const {
    return url.find("file://") == 0;
}

bool Browser::validateFileUrl(const std::string& url) const {
    if (!isFileUrl(url)) {
        return false;
    }
    
    // Extract path after file://
    std::string path = url.substr(7);
    if (path.empty()) {
        return false;
    }
    
    // Handle localhost prefix in file URLs
    if (path.rfind("localhost/", 0) == 0) {
        path = path.substr(10);  // Remove "localhost/"
    }
    
    // Normalize path separators to forward slashes for consistent checking
    std::replace(path.begin(), path.end(), '\\', '/');
    
    // Security checks: reject dangerous system paths (more comprehensive)
    std::vector<std::string> dangerous_paths = {
        "/etc/",
        "/proc/",
        "/sys/",
        "/dev/",
        "/root/",
        "/usr/bin/",
        "/usr/sbin/",
        "/sbin/",
        "/bin/",
        "/var/log/",
        "/boot/",
        "C:/Windows/",
        "C:/Program Files/",
        "C:/Users/Administrator/",
        "C:/System32/",
        "C:/ProgramData/"
    };
    
    // Convert path to lowercase for case-insensitive comparison on Windows paths
    std::string path_lower = path;
    std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(), ::tolower);
    
    for (const auto& dangerous : dangerous_paths) {
        std::string dangerous_lower = dangerous;
        std::transform(dangerous_lower.begin(), dangerous_lower.end(), dangerous_lower.begin(), ::tolower);
        
        if (path_lower.find(dangerous_lower) == 0) {
            return false;
        }
    }
    
    // Check for path traversal attempts (more comprehensive)
    if (path.find("../") != std::string::npos || 
        path.find("..\\") != std::string::npos ||
        path.find("/..") != std::string::npos ||
        path.find("\\..") != std::string::npos ||
        path.find("..") == 0) {  // Path starting with ..
        return false;
    }
    
    // Check for null bytes and other dangerous characters
    for (char c : path) {
        if (c == '\0' || c < 0x20) {
            return false;
        }
    }
    
    // Only allow HTML and related file extensions for file URLs
    if (path.find('.') != std::string::npos) {
        std::string extension;
        size_t last_dot = path.find_last_of('.');
        if (last_dot != std::string::npos) {
            extension = path.substr(last_dot + 1);
            
            // Convert to lowercase for comparison
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            // Only allow safe web-related extensions
            if (extension != "html" && extension != "htm" && extension != "xhtml") {
                return false;
            }
        }
    } else {
        // If no extension, reject (directories are not allowed)
        return false;
    }
    
    // For security, check if file actually exists and is readable
    // This prevents attempts to access non-existent system files
    try {
        std::filesystem::path file_path(path);
        if (!std::filesystem::exists(file_path) || !std::filesystem::is_regular_file(file_path)) {
            return false;
        }
        
        // Additional check: ensure it's not a symlink to dangerous location
        std::filesystem::path canonical_path = std::filesystem::canonical(file_path);
        std::string canonical_str = canonical_path.string();
        std::replace(canonical_str.begin(), canonical_str.end(), '\\', '/');
        std::transform(canonical_str.begin(), canonical_str.end(), canonical_str.begin(), ::tolower);
        
        for (const auto& dangerous : dangerous_paths) {
            std::string dangerous_lower = dangerous;
            std::transform(dangerous_lower.begin(), dangerous_lower.end(), dangerous_lower.begin(), ::tolower);
            
            if (canonical_str.find(dangerous_lower) == 0) {
                return false;
            }
        }
        
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

// ========== Viewport and User Agent Methods ==========

std::pair<int, int> Browser::getViewport() const {
    // Get current window size as the viewport
    int width, height;
    gtk_window_get_default_size(GTK_WINDOW(window), &width, &height);
    
    // If not set, return reasonable defaults
    if (width <= 0) width = 1920;
    if (height <= 0) height = 1080;
    
    return std::make_pair(width, height);
}

void Browser::setViewport(int width, int height) {
    debug_output("Setting viewport to: " + std::to_string(width) + "x" + std::to_string(height));
    
    // Set the GTK window size
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    
    // Set the WebView widget size request
    gtk_widget_set_size_request(GTK_WIDGET(webView), width, height);
    
    // Process any pending size allocation events
    while (g_main_context_pending(g_main_context_default())) {
        g_main_context_iteration(g_main_context_default(), FALSE);
    }
}

void Browser::setUserAgent(const std::string& userAgent) {
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_user_agent(settings, userAgent.c_str());
}


void Browser::ensureProperViewportForScreenshots() {
    // Get current viewport settings using the proper API
    auto [width, height] = getViewport();
    
    debug_output("Ensuring viewport for screenshots: " + std::to_string(width) + "x" + std::to_string(height));
    
    // Set window and WebView size - but keep window HIDDEN
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    gtk_widget_set_size_request(GTK_WIDGET(webView), width, height);
    
    // Realize widgets without showing them
    gtk_widget_realize(window);
    gtk_widget_realize(GTK_WIDGET(webView));
    
    // Force size allocation without visibility
    GtkAllocation allocation;
    allocation.x = 0;
    allocation.y = 0;
    allocation.width = width;
    allocation.height = height;
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation, -1);
    
    // Process pending events to ensure proper layout
    while (g_main_context_pending(g_main_context_default())) {
        g_main_context_iteration(g_main_context_default(), FALSE);
    }
    
    // Give WebKit time to render offscreen
    wait(200);
    
    // Set viewport meta tag in the page if possible
    std::string jsViewport = "(function() { "
        "try { "
        "  var meta = document.querySelector('meta[name=\"viewport\"]'); "
        "  if (!meta) { "
        "    meta = document.createElement('meta'); "
        "    meta.name = 'viewport'; "
        "    document.head.appendChild(meta); "
        "  } "
        "  meta.content = 'width=" + std::to_string(width) + ",initial-scale=1.0'; "
        "  return 'viewport_set'; "
        "} catch(e) { "
        "  return 'viewport_error'; "
        "} "
        "})()";
    
    executeJavascriptSync(jsViewport);
}

