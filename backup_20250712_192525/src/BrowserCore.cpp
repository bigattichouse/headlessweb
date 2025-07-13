#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <stdexcept>
#include <iostream>

// External debug flag
extern bool g_debug;

// ========== Navigation Methods ==========

void Browser::loadUri(const std::string& uri) {
    // Validate URL format before attempting to load
    if (uri.empty()) {
        throw std::invalid_argument("Error: Empty URL provided");
    }
    
    // Basic URL validation
    if (uri.find("://") == std::string::npos) {
        throw std::invalid_argument("Error: Invalid URL format (missing protocol): " + uri);
    }
    
    // Check for clearly invalid protocols
    std::string protocol = uri.substr(0, uri.find("://"));
    if (protocol != "http" && protocol != "https" && protocol != "file" && protocol != "ftp") {
        // Allow some other common protocols but warn about unknown ones
        if (protocol != "data" && protocol != "about" && protocol != "javascript") {
            throw std::invalid_argument("Error: Invalid URL protocol '" + protocol + "': " + uri);
        }
    }
    
    // Additional validation for file URLs
    if (protocol == "file") {
        std::string path = uri.substr(7); // Remove "file://"
        if (path.empty()) {
            throw std::invalid_argument("Error: Invalid file URL (empty path): " + uri);
        }
    }
    
    debug_output("Loading URI: " + uri);
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
    
    // Check for protocol
    if (url.find("://") == std::string::npos) {
        return false;
    }
    
    // Extract and validate protocol
    std::string protocol = url.substr(0, url.find("://"));
    if (protocol != "http" && protocol != "https" && protocol != "file" && 
        protocol != "ftp" && protocol != "data" && protocol != "about" && 
        protocol != "javascript") {
        return false;
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
    return !path.empty();
}

// ========== Viewport and User Agent Methods ==========

void Browser::setViewport(int width, int height) {
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
}

void Browser::setUserAgent(const std::string& userAgent) {
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_user_agent(settings, userAgent.c_str());
}
