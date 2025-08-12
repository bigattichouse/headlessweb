#include <iostream>
#include <sstream>

// Full reproduction of the Framework Detection Script generation
std::string generateFrameworkDetectionScript(const std::string& framework) {
    std::ostringstream script;
    
    script << R"JS(
(function(targetFramework) {
    // HeadlessWeb Framework Detection
    var frameworks = {
        react: function() {
            return typeof window.React !== 'undefined' || 
                   document.querySelector('[data-reactroot]') !== null ||
                   document.querySelector('._reactContainer') !== null;
        },
        
        vue: function() {
            return typeof window.Vue !== 'undefined' ||
                   document.querySelector('[data-v-]') !== null ||
                   document.querySelector('.__vue__') !== null;
        },
        
        angular: function() {
            return typeof window.angular !== 'undefined' ||
                   typeof window.ng !== 'undefined' ||
                   document.querySelector('[ng-app]') !== null ||
                   document.querySelector('app-root') !== null;
        }
    };
    
    var detected_frameworks = [];
    
    // Check each framework
    for (var name in frameworks) {
        if (frameworks[name]()) {
            detected_frameworks.push(name);
        }
    }
    
    if (detected_frameworks.length > 0 && typeof window.hweb_emit_page_event === 'function') {
        window.hweb_emit_page_event('FRAMEWORK_DETECTED', window.location.href, 1.0, detected_frameworks.join(','));
    }
    
    return detected_frameworks;
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