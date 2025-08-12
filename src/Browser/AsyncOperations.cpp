#include "Browser.h"
#include <future>
#include <chrono>

// Implementation of async waiting methods for the Browser class

std::future<bool> Browser::waitForBrowserReady(int timeout_ms) {
    if (!state_manager_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return state_manager_->waitForMinimumState(BrowserEvents::BrowserState::FULLY_READY, timeout_ms);
}

std::future<bool> Browser::waitForDOMReady(int timeout_ms) {
    if (!state_manager_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return state_manager_->waitForMinimumState(BrowserEvents::BrowserState::DOM_READY, timeout_ms);
}

std::future<bool> Browser::waitForElementAsync(const std::string& selector, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!mutation_tracker_ || !event_bus_) {
        promise->set_value(false);
        return future;
    }
    
    // First check if element already exists
    std::string check_script = R"JS(
        (function() {
)JS";
    check_script += "            let element = document.querySelector('" + selector + "');\n";
    check_script += R"JS(
            return element !== null;
        })();
    )JS";
    
    std::string result = executeJavascriptSync(check_script);
    if (result == "true") {
        promise->set_value(true);
        return future;
    }
    
    // Set up mutation observer to wait for element
    auto dom_future = mutation_tracker_->waitForElementAdd(selector, timeout_ms);
    
    // Convert DOMEvent future to bool future
    std::thread([promise, dom_future = std::move(dom_future)]() mutable {
        try {
            auto dom_event = dom_future.get();
            promise->set_value(true);
        } catch (const std::exception&) {
            promise->set_value(false);
        }
    }).detach();
    
    return future;
}

std::future<bool> Browser::waitForNavigationAsync(int timeout_ms) {
    if (!event_bus_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    auto nav_future = event_bus_->waitForNavigation(timeout_ms);
    
    // Convert NavigationEvent future to bool future
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    std::thread([promise, nav_future = std::move(nav_future)]() mutable {
        try {
            auto nav_event = nav_future.get();
            promise->set_value(nav_event.success);
        } catch (const std::exception&) {
            promise->set_value(false);
        }
    }).detach();
    
    return future;
}

std::future<bool> Browser::waitForNetworkIdleAsync(int idle_time_ms, int timeout_ms) {
    if (!network_tracker_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return network_tracker_->waitForNetworkIdle(idle_time_ms, timeout_ms);
}

// ========== Enhanced Readiness Detection Methods ==========

std::future<bool> Browser::waitForPageFullyReady(int timeout_ms) {
    if (!readiness_tracker_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return readiness_tracker_->waitForFullReadiness(timeout_ms);
}

std::future<bool> Browser::waitForPageBasicReady(int timeout_ms) {
    if (!readiness_tracker_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return readiness_tracker_->waitForBasicReadiness(timeout_ms);
}

std::future<bool> Browser::waitForPageInteractive(int timeout_ms) {
    if (!readiness_tracker_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return readiness_tracker_->waitForInteractive(timeout_ms);
}

std::future<bool> Browser::waitForJavaScriptReadyAsync(int timeout_ms) {
    if (!readiness_tracker_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return readiness_tracker_->waitForJavaScriptReady(timeout_ms);
}

std::future<bool> Browser::waitForResourcesLoadedAsync(int timeout_ms) {
    if (!readiness_tracker_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return readiness_tracker_->waitForResourcesLoaded(timeout_ms);
}

// ========== Non-blocking Readiness Checking ==========

bool Browser::isPageFullyReady() const {
    if (!readiness_tracker_) return false;
    return readiness_tracker_->isFullyReady();
}

bool Browser::isPageBasicReady() const {
    if (!readiness_tracker_) return false;
    return readiness_tracker_->isBasicReady();
}

bool Browser::isPageInteractive() const {
    if (!readiness_tracker_) return false;
    return readiness_tracker_->isInteractive();
}

// ========== Event-Driven DOM Operations ==========

std::future<bool> Browser::fillInputAsync(const std::string& selector, const std::string& value, int timeout_ms) {
    if (!async_dom_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    // Set up the async operation and execute the script
    auto future = async_dom_->fillInputAsync(selector, value, timeout_ms);
    
    // Execute the JavaScript to perform the operation and set up event monitoring
    try {
        std::string operation_id = "fill_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::string script = async_dom_->generateInputFillScript(selector, value, operation_id);
        std::string result = executeJavascriptSync(script);
        
        // The JavaScript will emit events that the async operation is waiting for
        
    } catch (const std::exception& e) {
        // If JavaScript execution fails, return false future
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return std::move(future);
}

std::future<bool> Browser::clickElementAsync(const std::string& selector, int timeout_ms) {
    if (!async_dom_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    // Set up the async operation and execute the script
    auto future = async_dom_->clickElementAsync(selector, timeout_ms);
    
    // Execute the JavaScript to perform the operation
    try {
        std::string operation_id = "click_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::string script = async_dom_->generateClickScript(selector, operation_id);
        std::string result = executeJavascriptSync(script);
        
    } catch (const std::exception& e) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return std::move(future);
}

std::future<bool> Browser::selectOptionAsync(const std::string& selector, const std::string& value, int timeout_ms) {
    if (!async_dom_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    auto future = async_dom_->selectOptionAsync(selector, value, timeout_ms);
    
    try {
        std::string operation_id = "select_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::string script = async_dom_->generateSelectScript(selector, value, operation_id);
        std::string result = executeJavascriptSync(script);
        
    } catch (const std::exception& e) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return std::move(future);
}

std::future<bool> Browser::submitFormAsync(const std::string& selector, int timeout_ms) {
    if (!async_dom_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_dom_->submitFormAsync(selector, timeout_ms);
}

std::future<bool> Browser::checkElementAsync(const std::string& selector, int timeout_ms) {
    if (!async_dom_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_dom_->checkElementAsync(selector, timeout_ms);
}

std::future<bool> Browser::uncheckElementAsync(const std::string& selector, int timeout_ms) {
    if (!async_dom_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_dom_->uncheckElementAsync(selector, timeout_ms);
}

std::future<bool> Browser::focusElementAsync(const std::string& selector, int timeout_ms) {
    if (!async_dom_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_dom_->focusElementAsync(selector, timeout_ms);
}

// ========== Event-Driven Navigation Operations ==========

std::future<bool> Browser::waitForPageLoadCompleteAsync(const std::string& url, int timeout_ms) {
    if (!async_nav_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_nav_->waitForPageLoadComplete(url, timeout_ms);
}

std::future<bool> Browser::waitForViewportReadyAsync(int timeout_ms) {
    if (!async_nav_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_nav_->waitForViewportReady(timeout_ms);
}

std::future<bool> Browser::waitForRenderingCompleteAsync(int timeout_ms) {
    if (!async_nav_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_nav_->waitForRenderingComplete(timeout_ms);
}

std::future<bool> Browser::waitForSPANavigationAsync(const std::string& route, int timeout_ms) {
    if (!async_nav_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_nav_->waitForSPANavigation(route, timeout_ms);
}

std::future<bool> Browser::waitForFrameworkReadyAsync(const std::string& framework, int timeout_ms) {
    if (!async_nav_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_nav_->waitForFrameworkReady(framework, timeout_ms);
}

// ========== Event-Driven Session Operations ==========

std::future<bool> Browser::waitForUserAgentSetAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForUserAgentSet(timeout_ms);
}

std::future<bool> Browser::waitForViewportSetAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForViewportSet(timeout_ms);
}

std::future<bool> Browser::waitForCookiesRestoredAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForCookiesRestored(timeout_ms);
}

std::future<bool> Browser::waitForStorageRestoredAsync(const std::string& storage_type, int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForStorageRestored(storage_type, timeout_ms);
}

std::future<bool> Browser::waitForFormStateRestoredAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForFormStateRestored(timeout_ms);
}

std::future<bool> Browser::waitForActiveElementsRestoredAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForActiveElementsRestored(timeout_ms);
}

std::future<bool> Browser::waitForCustomAttributesRestoredAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForCustomAttributesRestored(timeout_ms);
}

std::future<bool> Browser::waitForCustomStateRestoredAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForCustomStateRestored(timeout_ms);
}

std::future<bool> Browser::waitForScrollPositionsRestoredAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForScrollPositionsRestored(timeout_ms);
}

std::future<bool> Browser::waitForSessionRestorationCompleteAsync(int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->waitForSessionRestorationComplete(timeout_ms);
}

std::future<bool> Browser::restoreSessionAsync(const Session& session, int timeout_ms) {
    if (!async_session_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_value(false);
        return promise->get_future();
    }
    
    return async_session_->restoreSessionAsync(session.getName(), timeout_ms);
}