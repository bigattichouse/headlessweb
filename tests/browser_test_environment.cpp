#include "browser_test_environment.h"
#include <iostream>
#include <cstdlib>
#include <gtk/gtk.h>

std::unique_ptr<Browser> g_browser = nullptr;

void BrowserTestEnvironment::SetUp() {
    debug_output("Global BrowserTestEnvironment SetUp called.");
    
    // Set environment variables to prevent GTK file dialogs and desktop integration
    setenv("GDK_BACKEND", "broadway", 1);  // Use headless backend
    setenv("GTK_RECENT_FILES_ENABLED", "0", 1);  // Disable recent files access
    setenv("GTK_RECENT_FILES_MAX_AGE", "0", 1);  // Set recent files max age to 0
    setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", 1);  // Reduce desktop integration
    setenv("WEBKIT_DISABLE_DMABUF_RENDERER", "1", 1);  // Disable DMA buffer renderer
    setenv("XDG_CONFIG_HOME", "/tmp/headless_gtk_config", 1);  // Use temp config directory
    setenv("XDG_DATA_HOME", "/tmp/headless_gtk_data", 1);  // Use temp data directory
    
    debug_output("Set environment variables for headless GTK operation");
    
    if (!gtk_is_initialized()) {
        gtk_init();
    }
    HWeb::HWebConfig test_config;
    test_config.allow_data_uri = true;
    g_browser = std::make_unique<Browser>(test_config);
    debug_output("Global Browser instance created.");
}

void BrowserTestEnvironment::TearDown() {
    debug_output("Global BrowserTestEnvironment TearDown called.");
    g_browser.reset();
    debug_output("Global Browser instance destroyed.");
}
