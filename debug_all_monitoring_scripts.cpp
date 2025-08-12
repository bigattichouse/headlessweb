#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Simulate all three monitoring scripts as called in Events.cpp lines 74-76

std::string generateSPANavigationDetectionScript() {
    return R"JS(
(function() {
    // HeadlessWeb SPA Navigation Detection
    if (typeof window.hweb_spa_monitor === 'undefined') {
        window.hweb_spa_monitor = {
            current_url: window.location.href,
            current_hash: window.location.hash,
            
            init: function() {
                // Monitor pushState/replaceState for SPA navigation
                var originalPushState = history.pushState;
                var originalReplaceState = history.replaceState;
                var self = this;
                
                history.pushState = function() {
                    originalPushState.apply(history, arguments);
                    self.onURLChange();
                };
                
                history.replaceState = function() {
                    originalReplaceState.apply(history, arguments);
                    self.onURLChange();
                };
                
                // Monitor popstate (back/forward buttons)
                window.addEventListener('popstate', function() {
                    self.onURLChange();
                });
                
                // Monitor hash changes
                window.addEventListener('hashchange', function() {
                    self.onHashChange();
                });
            },
            
            onURLChange: function() {
                var new_url = window.location.href;
                if (new_url !== this.current_url) {
                    var old_url = this.current_url;
                    this.current_url = new_url;
                    
                    if (typeof window.hweb_emit_page_event === 'function') {
                        window.hweb_emit_page_event('SPA_ROUTE_CHANGED', new_url, 1.0, old_url);
                    }
                }
            },
            
            onHashChange: function() {
                var new_hash = window.location.hash;
                if (new_hash !== this.current_hash) {
                    var old_hash = this.current_hash;
                    this.current_hash = new_hash;
                    
                    if (typeof window.hweb_emit_page_event === 'function') {
                        window.hweb_emit_page_event('SPA_ROUTE_CHANGED', window.location.href, 1.0, old_hash);
                    }
                }
            }
        };
        
        // Initialize SPA monitoring
        window.hweb_spa_monitor.init();
    }
    
    return {
        current_url: window.hweb_spa_monitor.current_url,
        current_hash: window.hweb_spa_monitor.current_hash
    };
})();
)JS";
}

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

std::string generateRenderingCompleteScript() {
    return R"JS(
(function() {
    // HeadlessWeb Rendering Completion Detection
    if (typeof window.hweb_rendering_monitor === 'undefined') {
        window.hweb_rendering_monitor = {
            last_dom_change: Date.now(),
            observer: null,
            check_interval: null,
            stability_threshold: 500, // ms
            
            init: function() {
                var self = this;
                
                // Set up MutationObserver to detect DOM changes
                this.observer = new MutationObserver(function(mutations) {
                    self.last_dom_change = Date.now();
                });
                
                this.observer.observe(document.body || document.documentElement, {
                    childList: true,
                    subtree: true,
                    attributes: true,
                    characterData: true
                });
                
                // Check for stability periodically
                this.check_interval = setInterval(function() {
                    self.checkStability();
                }, 100);
            },
            
            checkStability: function() {
                var now = Date.now();
                var time_since_change = now - this.last_dom_change;
                
                // If DOM has been stable for threshold time, consider rendering complete
                if (time_since_change >= this.stability_threshold) {
                    if (typeof window.hweb_emit_page_event === 'function') {
                        window.hweb_emit_page_event('RENDERING_COMPLETE', window.location.href, 1.0);
                    }
                    
                    // Clean up
                    if (this.observer) {
                        this.observer.disconnect();
                        this.observer = null;
                    }
                    if (this.check_interval) {
                        clearInterval(this.check_interval);
                        this.check_interval = null;
                    }
                }
            },
            
            getStatus: function() {
                return {
                    time_since_change: Date.now() - this.last_dom_change,
                    stability_threshold: this.stability_threshold,
                    is_stable: (Date.now() - this.last_dom_change) >= this.stability_threshold
                };
            }
        };
        
        // Initialize rendering monitoring
        window.hweb_rendering_monitor.init();
    }
    
    return window.hweb_rendering_monitor.getStatus();
})();
)JS";
}

std::vector<std::string> splitLines(const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream stream(str);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

int main() {
    std::cout << "=== DEBUGGING ALL THREE MONITORING SCRIPTS ===" << std::endl;
    
    // Test all three scripts as called in Events.cpp lines 74-76
    std::string script1 = generateSPANavigationDetectionScript();
    std::string script2 = generateFrameworkDetectionScript("");
    std::string script3 = generateRenderingCompleteScript();
    
    std::cout << "\n=== SCRIPT 1: SPA Navigation Detection ===" << std::endl;
    auto lines1 = splitLines(script1);
    std::cout << "Line count: " << lines1.size() << std::endl;
    for (size_t i = 0; i < lines1.size() && i < 70; ++i) {
        std::cout << (i+1) << ": " << lines1[i] << std::endl;
    }
    
    std::cout << "\n=== SCRIPT 2: Framework Detection ===" << std::endl;
    auto lines2 = splitLines(script2);
    std::cout << "Line count: " << lines2.size() << std::endl;
    for (size_t i = 0; i < lines2.size() && i < 70; ++i) {
        std::cout << (i+1) << ": " << lines2[i] << std::endl;
    }
    
    std::cout << "\n=== SCRIPT 3: Rendering Complete ===" << std::endl;
    auto lines3 = splitLines(script3);
    std::cout << "Line count: " << lines3.size() << std::endl;
    for (size_t i = 0; i < lines3.size() && i < 70; ++i) {
        std::cout << (i+1) << ": " << lines3[i] << std::endl;
    }
    
    // Combined script simulation
    std::string combined = script1 + "\n" + script2 + "\n" + script3;
    auto combined_lines = splitLines(combined);
    
    std::cout << "\n=== COMBINED SCRIPT ANALYSIS ===" << std::endl;
    std::cout << "Total combined line count: " << combined_lines.size() << std::endl;
    
    // Focus on line 59 area
    std::cout << "\n=== LINES 55-65 (around line 59) ===" << std::endl;
    for (size_t i = 54; i < 65 && i < combined_lines.size(); ++i) {
        std::cout << (i+1) << ": " << combined_lines[i] << std::endl;
    }
    
    return 0;
}