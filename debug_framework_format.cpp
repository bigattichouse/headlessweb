#include <iostream>
#include <sstream>
#include <string>

// Test the exact format and structure of Framework Detection script
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
)JS";
    script << ")('" << framework << "');";
    
    return script.str();
}

int main() {
    std::string result = generateFrameworkDetectionScript("");
    
    std::cout << "=== CHECKING EXACT FORMAT AND STRUCTURE ===" << std::endl;
    std::cout << "Character-by-character analysis of the end:" << std::endl;
    
    // Focus on the ending where the issue likely is
    size_t len = result.length();
    size_t start = len > 100 ? len - 100 : 0;
    
    for (size_t i = start; i < len; ++i) {
        char c = result[i];
        std::cout << "Pos " << i << ": ";
        if (c == '\n') {
            std::cout << "\\n (newline)" << std::endl;
        } else if (c == '\r') {
            std::cout << "\\r (carriage return)" << std::endl;
        } else if (c == '\t') {
            std::cout << "\\t (tab)" << std::endl;
        } else if (c == ' ') {
            std::cout << "' ' (space)" << std::endl;
        } else {
            std::cout << "'" << c << "'" << std::endl;
        }
    }
    
    std::cout << "\n=== FULL SCRIPT ===" << std::endl;
    std::cout << result << std::endl;
    
    std::cout << "\n=== TESTING DIFFERENT FRAMEWORK VALUES ===" << std::endl;
    
    // Test with different framework values
    std::cout << "Empty string result length: " << generateFrameworkDetectionScript("").length() << std::endl;
    std::cout << "React result length: " << generateFrameworkDetectionScript("react").length() << std::endl;
    
    // Check for potential NULL termination or escaping issues
    std::string test_empty = generateFrameworkDetectionScript("");
    std::string test_react = generateFrameworkDetectionScript("react");
    
    std::cout << "Last 10 chars of empty: ";
    for (size_t i = test_empty.length() - 10; i < test_empty.length(); ++i) {
        std::cout << "'" << test_empty[i] << "' ";
    }
    std::cout << std::endl;
    
    std::cout << "Last 10 chars of react: ";
    for (size_t i = test_react.length() - 10; i < test_react.length(); ++i) {
        std::cout << "'" << test_react[i] << "' ";
    }
    std::cout << std::endl;
    
    return 0;
}