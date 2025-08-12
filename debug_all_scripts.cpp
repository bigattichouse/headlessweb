#include <iostream>
#include <sstream>

// Test all three monitoring scripts together
std::string generateSPANavigationDetectionScript() {
    return "// SPA Navigation Detection Script placeholder\nconsole.log('SPA detection loaded');";
}

std::string generateFrameworkDetectionScript(const std::string& framework) {
    std::ostringstream script;
    
    script << R"JS(
(function(targetFramework) {
    console.log('Framework detection called with:', targetFramework);
    return [];
}
)JS";
    script << ")('" << framework << "');";
    
    return script.str();
}

std::string generateRenderingCompleteScript() {
    return "// Rendering Complete Script placeholder\nconsole.log('Rendering complete loaded');";
}

int main() {
    std::cout << "=== Testing all three monitoring scripts ===" << std::endl;
    
    // Simulate what happens in Events.cpp lines 74-76
    std::string script1 = generateSPANavigationDetectionScript();
    std::string script2 = generateFrameworkDetectionScript("");
    std::string script3 = generateRenderingCompleteScript();
    
    std::cout << "Script 1 (SPA):" << std::endl;
    std::cout << script1 << std::endl << std::endl;
    
    std::cout << "Script 2 (Framework Detection):" << std::endl;
    std::cout << script2 << std::endl << std::endl;
    
    std::cout << "Script 3 (Rendering Complete):" << std::endl;
    std::cout << script3 << std::endl << std::endl;
    
    // Count total lines if they were combined
    int total_lines = 0;
    for (char c : script1 + script2 + script3) {
        if (c == '\n') total_lines++;
    }
    
    std::cout << "Total lines if combined: " << total_lines << std::endl;
    
    return 0;
}