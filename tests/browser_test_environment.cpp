#include "browser_test_environment.h"
#include <iostream>
#include <cstdlib>
#include <gtk/gtk.h>

std::unique_ptr<Browser> g_browser = nullptr;

void BrowserTestEnvironment::SetUp() {
    debug_output("Global BrowserTestEnvironment SetUp called.");
    
    // Set comprehensive environment variables for headless operation
    // Don't force a specific backend, let GTK choose the best available one
    setenv("GTK_RECENT_FILES_ENABLED", "0", 1);  // Disable recent files access
    setenv("GTK_RECENT_FILES_MAX_AGE", "0", 1);  // Set recent files max age to 0
    setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", 1);  // Reduce desktop integration
    setenv("WEBKIT_DISABLE_DMABUF_RENDERER", "1", 1);  // Disable DMA buffer renderer
    setenv("XDG_CONFIG_HOME", "/tmp/headless_gtk_config", 1);  // Use temp config directory
    setenv("XDG_DATA_HOME", "/tmp/headless_gtk_data", 1);  // Use temp data directory
    setenv("XDG_RUNTIME_DIR", "/tmp/headless_runtime", 1);  // Runtime directory
    setenv("TMPDIR", "/tmp", 1);  // Temp directory
    setenv("HOME", "/tmp/headless_home", 1);  // Temporary home directory
    
    // Disable accessibility features that might require display
    setenv("NO_AT_BRIDGE", "1", 1);
    setenv("GTK_A11Y", "none", 1);
    
    // Additional file dialog prevention measures
    setenv("WEBKIT_DISABLE_FILE_PICKER", "1", 1);  // Disable WebKit file picker
    setenv("GTK_FILE_CHOOSER_BACKEND", "none", 1);  // Force no file chooser backend
    setenv("GIO_USE_VFS", "local", 1);  // Use local VFS only
    setenv("GVFS_DISABLE_FUSE", "1", 1);  // Disable GVFS FUSE mounting
    
    // Create necessary directories
    system("mkdir -p /tmp/headless_gtk_config");
    system("mkdir -p /tmp/headless_gtk_data"); 
    system("mkdir -p /tmp/headless_runtime");
    system("mkdir -p /tmp/headless_home");
    
    debug_output("Set environment variables for headless GTK operation");
    
    // Initialize GTK with error handling
    if (!gtk_is_initialized()) {
        if (!gtk_init_check()) {
            debug_output("GTK initialization failed, continuing anyway");
        } else {
            debug_output("GTK initialized successfully in headless mode");
        }
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
