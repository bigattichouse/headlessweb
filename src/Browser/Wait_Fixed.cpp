// CRITICAL FIX: Simplified, reliable wait methods for Browser Wait tests
// This file contains the corrected waitForSPANavigation method

bool Browser::waitForSPANavigation(const std::string& route, int timeout_ms) {
    debug_output("Waiting for SPA navigation to: " + (route.empty() ? "any route" : route));
    
    std::string initial_url = getCurrentUrl();
    debug_output("Initial URL: " + initial_url);
    
    int elapsed = 0;
    const int check_interval = 50; // Check every 50ms for responsiveness
    
    while (elapsed < timeout_ms) {
        // Get current URL directly
        std::string current_url = getCurrentUrl();
        
        if (route.empty()) {
            // Wait for ANY navigation change
            if (current_url != initial_url) {
                debug_output("Navigation change detected: " + initial_url + " -> " + current_url);
                return true;
            }
            
            // Also check for hash changes via JavaScript
            std::string hash_check = executeJavascriptSync(
                "(function() {"
                "  try {"
                "    var hash = window.location.hash;"
                "    var path = window.location.pathname;"
                "    return path + hash;"
                "  } catch(e) { return ''; }"
                "})()"
            );
            
            if (!hash_check.empty() && hash_check != initial_url) {
                debug_output("Hash/path change detected: " + hash_check);
                return true;
            }
        } else {
            // Wait for specific route
            if (current_url.find(route) != std::string::npos) {
                debug_output("Route found in URL: " + route + " in " + current_url);
                return true;
            }
            
            // Check via JavaScript for hash/path matches
            std::string route_check = executeJavascriptSync(
                "(function() {"
                "  try {"
                "    var hash = window.location.hash;"
                "    var path = window.location.pathname;"
                "    var route = '" + route + "';"
                "    return (hash.indexOf(route) !== -1 || path.indexOf(route) !== -1);"
                "  } catch(e) { return false; }"
                "})()"
            );
            
            if (route_check == "true" || route_check == "1") {
                debug_output("Route found via JavaScript: " + route);
                return true;
            }
        }
        
        wait(check_interval);
        elapsed += check_interval;
    }
    
    debug_output("SPA navigation timeout: " + route);
    return false;
}