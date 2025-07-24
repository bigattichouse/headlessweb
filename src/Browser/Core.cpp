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

bool Browser::validateUrl(const std::string& url) const {
    if (url.empty()) {
        return false;
    }
    
    // Check for minimum URL length
    if (url.length() < 10) { // Minimum: "http://a.b"
        return false;
    }
    
    // Check for protocol separator
    size_t protocol_pos = url.find("://");
    if (protocol_pos == std::string::npos) {
        return false;
    }
    
    // Extract and validate protocol
    std::string protocol = url.substr(0, protocol_pos);
    
    // Only allow safe protocols - reject dangerous ones
    if (protocol != "http" && protocol != "https" && protocol != "file") {
        return false; // Reject ftp, javascript, data, etc.
    }
    
    // Validate what comes after the protocol
    std::string remainder = url.substr(protocol_pos + 3);
    if (remainder.empty()) {
        return false; // Reject malformed URLs like "http://"
    }
    
    // Check for dangerous characters
    if (url.find('\0') != std::string::npos || 
        url.find('\x01') != std::string::npos || 
        url.find('\x02') != std::string::npos) {
        return false; // Reject binary data
    }
    
    // Additional validation for HTTP/HTTPS URLs
    if (protocol == "http" || protocol == "https") {
        // Must have host part after ://
        if (remainder.length() < 2) { // Minimum: "ab"
            return false;
        }
        
        // Check for valid domain characters in basic host part
        size_t path_start = remainder.find('/');
        std::string host_part = (path_start != std::string::npos) ? 
                               remainder.substr(0, path_start) : remainder;
        
        // Reject obvious malformed hosts
        if (host_part.empty() || host_part == "." || host_part.find("..") != std::string::npos) {
            return false;
        }
        
        // Reject non-ASCII domains for security (would need IDN validation)
        for (char c : host_part) {
            if (static_cast<unsigned char>(c) > 127) {
                return false;
            }
        }
    }
    
    // Special file URL validation
    if (protocol == "file") {
        return validateFileUrl(url);
    }
    
    return true;
}

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
    
    // Security checks: reject dangerous system paths
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
        "C:/Windows/",
        "C:/Program Files/",
        "C:/Users/Administrator/",
        "C:/System32/"
    };
    
    for (const auto& dangerous : dangerous_paths) {
        if (path.find(dangerous) == 0) {
            return false;
        }
    }
    
    // Check for path traversal attempts
    if (path.find("../") != std::string::npos || 
        path.find("..\\") != std::string::npos ||
        path.find("/..") != std::string::npos ||
        path.find("\\..") != std::string::npos) {
        return false;
    }
    
    // Check for null bytes and other dangerous characters
    if (path.find('\0') != std::string::npos ||
        path.find('\x01') != std::string::npos ||
        path.find('\x02') != std::string::npos) {
        return false;
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
    }
    
    // Check if file actually exists and is readable
    try {
        std::filesystem::path file_path(path);
        if (!std::filesystem::exists(file_path) || !std::filesystem::is_regular_file(file_path)) {
            return false;
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

