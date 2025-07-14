#include "Browser.h"
#include "WebKitCompat.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <iostream>
#include <filesystem>

// External debug flag
extern bool g_debug;

Browser::Browser() : cookieManager(nullptr), main_loop(g_main_loop_new(NULL, FALSE)) {
    gtk_init();
    
    // Initialize with a proper data manager for cookie/storage persistence
    std::string home = std::getenv("HOME");
    sessionDataPath = home + "/.hweb-poc/webkit-data";
    std::filesystem::create_directories(sessionDataPath);
    
    // Create WebKit settings first
    WebKitSettings* settings = webkit_settings_new();
    webkit_settings_set_enable_media(settings, FALSE);
    webkit_settings_set_enable_media_stream(settings, FALSE);
    webkit_settings_set_enable_webaudio(settings, FALSE);
    webkit_settings_set_enable_javascript(settings, TRUE);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_page_cache(settings, TRUE);
    webkit_settings_set_enable_html5_local_storage(settings, TRUE);
    webkit_settings_set_enable_html5_database(settings, TRUE);
    webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
    webkit_settings_set_allow_universal_access_from_file_urls(settings, TRUE);
    
    // Create web context with custom data directory
    WebKitWebContext* context = webkit_web_context_get_default();
    webkit_web_context_set_cache_model(context, WEBKIT_CACHE_MODEL_WEB_BROWSER);
    
    // Create data directories for persistent storage
    std::string dataDir = sessionDataPath + "/data";
    std::string cacheDir = sessionDataPath + "/cache";
    std::filesystem::create_directories(dataDir);
    std::filesystem::create_directories(cacheDir);
    
    // Create web view
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    if (!webView) {
        std::cerr << "Failed to create WebKit web view" << std::endl;
        exit(1);
    }
    
    // Apply settings to the web view
    webkit_web_view_set_settings(webView, settings);
    
    // Get the network session for cookie management
    cookieManager = WebKitCompat::getCookieManager(webView);
    if (cookieManager) {
        // Cookie persistence is handled automatically by modern WebKitGTK
        debug_output("Cookie manager initialized with automatic persistence");
    }
    
    // Create window but keep it ALWAYS HIDDEN for headless operation
    window = gtk_window_new();
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(webView));
    
    // Set window to be offscreen/headless - NEVER show it
    gtk_widget_set_visible(window, FALSE);
    
    // Set reasonable default size for offscreen rendering
    gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);
    gtk_widget_set_size_request(GTK_WIDGET(webView), 1920, 1080);
    
    // Store browser instance in the webView for callbacks
    g_object_set_data(G_OBJECT(webView), "browser-instance", this);
    
    // Setup event-driven signal handlers (implemented in BrowserEvents.cpp)
    setupSignalHandlers();
    
    g_object_unref(settings);
}

Browser::~Browser() {
    // Cleanup waiters (implemented in BrowserEvents.cpp)
    cleanupWaiters();
    
    if (main_loop) {
        g_main_loop_unref(main_loop);
    }
}
