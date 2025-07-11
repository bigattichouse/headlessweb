#pragma once

#include <iostream>
#include <string>

// Global debug flag
inline bool g_debug = false;

// Debug output function
inline void debug_output(const std::string& message) {
    if (g_debug) {
        std::cerr << "Debug: " << message << std::endl;
    }
}
