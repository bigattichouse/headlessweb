#include <gtk/gtk.h>
#include <iostream>
#include <chrono>

// Simple debug function to avoid dependency
void debug_output(const std::string& message) {
    std::cout << "[DEBUG] " << message << std::endl;
}

#include "src/Browser/EventLoopManager.h"

int main() {
    gtk_init();
    
    GMainLoop* main_loop = g_main_loop_new(NULL, FALSE);
    
    EventLoopManager manager;
    manager.initialize(main_loop);
    
    std::cout << "Testing EventLoopManager timeout..." << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    bool result = manager.waitForJavaScriptCompletion(1000); // 1 second timeout
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Result: " << result << ", Elapsed: " << elapsed << "ms" << std::endl;
    
    if (elapsed >= 900 && elapsed <= 1100 && !result) {
        std::cout << "SUCCESS: Timeout working correctly" << std::endl;
    } else {
        std::cout << "FAILURE: Unexpected behavior" << std::endl;
    }
    
    g_main_loop_unref(main_loop);
    return 0;
}