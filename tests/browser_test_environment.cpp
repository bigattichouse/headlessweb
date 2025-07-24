#include "browser_test_environment.h"
#include <iostream>
#include <gtk/gtk.h>

std::unique_ptr<Browser> g_browser = nullptr;

void BrowserTestEnvironment::SetUp() {
    debug_output("Global BrowserTestEnvironment SetUp called.");
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
