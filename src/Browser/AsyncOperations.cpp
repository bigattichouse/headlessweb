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
            let element = document.querySelector(')JS" + selector + R"JS(');
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