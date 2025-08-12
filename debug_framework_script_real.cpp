#include <iostream>
#include <sstream>

// Exact reproduction of the Framework Detection Script generation from AsyncNavigationOperations.cpp
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
        },
        
        jquery: function() {
            return typeof window.jQuery !== 'undefined' || typeof window.$ !== 'undefined';
        },
        
        backbone: function() {
            return typeof window.Backbone !== 'undefined';
        },
        
        ember: function() {
            return typeof window.Ember !== 'undefined';
        }
    };
    
    // Check specific framework or all frameworks
    if (targetFramework && frameworks[targetFramework.toLowerCase()]) {
        var detected = frameworks[targetFramework.toLowerCase()]();
        if (detected && typeof window.hweb_emit_page_event === 'function') {
            window.hweb_emit_page_event('FRAMEWORK_DETECTED', window.location.href, 1.0, targetFramework);
        }
        return detected;
    } else {
        // Check all frameworks
        var detected_frameworks = [];
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
})JS";
    script << "('" << framework << "');";
    
    return script.str();
}

int main() {
    // Test with empty framework string (as used in Events.cpp line 75)
    std::string result = generateFrameworkDetectionScript("");
    
    std::cout << "Real Generated JavaScript:" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << result << std::endl;
    std::cout << "===========================================" << std::endl;
    
    return 0;
}