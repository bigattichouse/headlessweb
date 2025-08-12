#include <iostream>
#include <sstream>

// Minimal reproduction of the Framework Detection Script generation
std::string generateFrameworkDetectionScript(const std::string& framework) {
    std::ostringstream script;
    
    script << R"JS(
(function(targetFramework) {
    // Simple test function
    console.log('Framework detection called with:', targetFramework);
    return true;
}
)JS";
    script << ")('" << framework << "');";
    
    return script.str();
}

int main() {
    // Test with empty framework string (as used in Events.cpp)
    std::string result = generateFrameworkDetectionScript("");
    
    std::cout << "Generated JavaScript:" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << result << std::endl;
    std::cout << "===========================================" << std::endl;
    
    return 0;
}