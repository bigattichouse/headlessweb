#include "NavigationService.h"
#include "../Output.h"

namespace HWeb {

NavigationService::NavigationService() {
}

NavigationService::~NavigationService() {
}

bool NavigationService::navigate_to_url(Browser& browser, const std::string& url) {
    try {
        browser.loadUri(url);
        return true;
    } catch (const std::invalid_argument& e) {
        Output::error(e.what());
        return false;
    } catch (const std::exception& e) {
        Output::error("Failed to navigate to " + url + ": " + e.what());
        return false;
    }
}

bool NavigationService::wait_for_navigation_complete(Browser& browser, int timeout_ms) {
    return browser.waitForNavigationSignal(timeout_ms);
}

bool NavigationService::wait_for_page_ready(Browser& browser, int timeout_ms) {
    return browser.waitForPageReadyEvent(timeout_ms);
}

NavigationStrategy NavigationService::determine_navigation_strategy(const HWebConfig& config, const Session& session) {
    if (!config.url.empty()) {
        return NavigationStrategy::NEW_URL;
    } else if (!session.getCurrentUrl().empty() && config.commands.empty() && config.assertions.empty()) {
        return NavigationStrategy::SESSION_RESTORE;
    } else if (!session.getCurrentUrl().empty() && (!config.commands.empty() || !config.assertions.empty())) {
        return NavigationStrategy::CONTINUE_SESSION;
    }
    
    return NavigationStrategy::NO_NAVIGATION;
}

NavigationService::NavigationPlan NavigationService::create_navigation_plan(const HWebConfig& config, const Session& session) {
    NavigationPlan plan;
    plan.strategy = determine_navigation_strategy(config, session);
    
    switch (plan.strategy) {
        case NavigationStrategy::NEW_URL:
            plan.should_navigate = true;
            plan.navigation_url = config.url;
            plan.is_session_restore = false;
            break;
            
        case NavigationStrategy::SESSION_RESTORE:
            plan.should_navigate = true;
            plan.navigation_url = session.getCurrentUrl();
            plan.is_session_restore = true;
            break;
            
        case NavigationStrategy::CONTINUE_SESSION:
            plan.should_navigate = true;
            plan.navigation_url = session.getCurrentUrl();
            plan.is_session_restore = true;
            break;
            
        case NavigationStrategy::NO_NAVIGATION:
        default:
            plan.should_navigate = false;
            plan.navigation_url = "";
            plan.is_session_restore = false;
            break;
    }
    
    return plan;
}

bool NavigationService::execute_navigation_plan(Browser& browser, Session& session, const NavigationPlan& plan) {
    if (!plan.should_navigate) {
        if (session.getCurrentUrl().empty()) {
            Output::error("No URL in session. Use --url to navigate.");
            return false;
        }
        return true; // No navigation needed
    }
    
    log_navigation_info(plan.navigation_url, plan.is_session_restore);
    
    if (!navigate_to_url(browser, plan.navigation_url)) {
        return false;
    }
    
    if (!wait_for_navigation_complete(browser, 15000)) {
        Output::error("Navigation timeout for: " + plan.navigation_url);
        return false;
    }
    
    if (!wait_for_page_ready(browser, 5000)) {
        Output::info("Warning: Page may not be fully ready, continuing...");
    }
    
    // Update session state for new URLs
    if (plan.strategy == NavigationStrategy::NEW_URL) {
        session.addToHistory(plan.navigation_url);
        session.setCurrentUrl(plan.navigation_url);
        Output::info("Navigated to " + plan.navigation_url);
    }
    
    // Restore session state if this is a session restore
    if (plan.is_session_restore) {
        browser.restoreSession(session);
    }
    
    return true;
}

void NavigationService::log_navigation_info(const std::string& url, bool is_restore) {
    if (is_restore) {
        Output::info("Restoring session URL: " + url);
    }
}

} // namespace HWeb