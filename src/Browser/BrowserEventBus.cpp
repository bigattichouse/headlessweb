#include "BrowserEventBus.h"
#include <algorithm>
#include <regex>
#include <thread>
#include <chrono>

namespace BrowserEvents {

// ========== BrowserEventBus Implementation ==========

size_t BrowserEventBus::subscribe(EventType type, EventHandler handler, EventCondition condition) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    size_t id = next_subscription_id_++;
    auto subscription = std::make_shared<Subscription>(id, type, std::move(handler), std::move(condition), false);
    
    subscriptions_[type].push_back(subscription);
    return id;
}

size_t BrowserEventBus::subscribeOnce(EventType type, EventHandler handler, EventCondition condition) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    size_t id = next_subscription_id_++;
    auto subscription = std::make_shared<Subscription>(id, type, std::move(handler), std::move(condition), true);
    
    subscriptions_[type].push_back(subscription);
    return id;
}

void BrowserEventBus::unsubscribe(size_t subscription_id) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    for (auto& [type, subs] : subscriptions_) {
        subs.erase(
            std::remove_if(subs.begin(), subs.end(),
                [subscription_id](const std::shared_ptr<Subscription>& sub) {
                    return sub->id == subscription_id;
                }),
            subs.end()
        );
    }
}

void BrowserEventBus::unsubscribeAll(EventType type) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_[type].clear();
}

void BrowserEventBus::clearAllSubscriptions() {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_.clear();
}

void BrowserEventBus::emit(const Event& event) {
    std::vector<std::shared_ptr<Subscription>> to_notify;
    std::vector<size_t> to_remove;
    
    // Collect subscriptions to notify (with lock)
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto it = subscriptions_.find(event.type);
        if (it != subscriptions_.end()) {
            for (const auto& sub : it->second) {
                // Check condition if provided
                if (!sub->condition || sub->condition(event)) {
                    to_notify.push_back(sub);
                    if (sub->once) {
                        to_remove.push_back(sub->id);
                    }
                }
            }
        }
    }
    
    // Notify handlers (without lock to avoid deadlock)
    for (const auto& sub : to_notify) {
        try {
            sub->handler(event);
        } catch (const std::exception& e) {
            // Log error but don't stop other handlers
            // TODO: Add proper logging
        }
    }
    
    // Remove one-time subscriptions
    for (size_t id : to_remove) {
        unsubscribe(id);
    }
    
    // Check if any promises are waiting for this event
    {
        std::lock_guard<std::mutex> lock(promises_mutex_);
        for (auto& [id, promise] : event_promises_) {
            if (!promise->isCompleted()) {
                promise->resolve(event);
            }
        }
    }
    
    // Cleanup completed promises periodically
    cleanupCompletedPromises();
}

void BrowserEventBus::emit(EventType type, const std::string& target, const std::string& data) {
    emit(Event(type, target, data));
}

std::future<Event> BrowserEventBus::waitForEvent(EventType type, int timeout_ms, EventCondition condition) {
    std::lock_guard<std::mutex> lock(promises_mutex_);
    
    size_t promise_id = next_promise_id_++;
    auto event_promise = std::make_shared<EventPromise<Event>>();
    event_promises_[promise_id] = event_promise;
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([this, promise_id, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            timeoutPromise(promise_id);
        }).detach();
    }
    
    return std::move(event_promise->getFuture());
}

std::future<NavigationEvent> BrowserEventBus::waitForNavigation(int timeout_ms, const std::string& expected_url) {
    auto promise = std::make_shared<EventPromise<NavigationEvent>>();
    
    EventCondition condition = nullptr;
    if (!expected_url.empty()) {
        condition = [expected_url](const Event& event) {
            if (auto nav_event = dynamic_cast<const NavigationEvent*>(&event)) {
                return nav_event->url.find(expected_url) != std::string::npos;
            }
            return false;
        };
    }
    
    subscribeOnce(EventType::NAVIGATION_COMPLETED, 
        [promise](const Event& event) {
            if (auto nav_event = dynamic_cast<const NavigationEvent*>(&event)) {
                promise->resolve(*nav_event);
            } else {
                // Convert generic event to NavigationEvent
                NavigationEvent converted_nav_event(event.type, event.target, "", true);
                promise->resolve(converted_nav_event);
            }
        }, condition);
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Navigation timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::future<DOMEvent> BrowserEventBus::waitForDOMChange(const std::string& selector, int timeout_ms) {
    auto promise = std::make_shared<EventPromise<DOMEvent>>();
    
    EventCondition condition = [selector](const Event& event) {
        return event.target == selector;
    };
    
    subscribeOnce(EventType::DOM_MUTATION, 
        [promise](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMEvent*>(&event)) {
                promise->resolve(*dom_event);
            } else {
                // Convert generic event to DOMEvent
                DOMEvent converted_dom_event(event.type, event.target);
                promise->resolve(converted_dom_event);
            }
        }, condition);
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("DOM change timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::future<NetworkEvent> BrowserEventBus::waitForNetworkIdle(int idle_time_ms, int timeout_ms) {
    auto promise = std::make_shared<EventPromise<NetworkEvent>>();
    
    subscribeOnce(EventType::NETWORK_IDLE, 
        [promise](const Event& event) {
            if (auto net_event = dynamic_cast<const NetworkEvent*>(&event)) {
                promise->resolve(*net_event);
            } else {
                // Convert generic event to NetworkEvent
                NetworkEvent converted_net_event(event.type, "", 0, "GET", true);
                promise->resolve(converted_net_event);
            }
        });
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Network idle timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

bool BrowserEventBus::hasSubscriptions(EventType type) const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(type);
    return it != subscriptions_.end() && !it->second.empty();
}

size_t BrowserEventBus::getSubscriptionCount(EventType type) const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(type);
    return it != subscriptions_.end() ? it->second.size() : 0;
}

void BrowserEventBus::cleanupCompletedPromises() {
    std::lock_guard<std::mutex> lock(promises_mutex_);
    
    auto it = event_promises_.begin();
    while (it != event_promises_.end()) {
        if (it->second->isCompleted()) {
            it = event_promises_.erase(it);
        } else {
            ++it;
        }
    }
}

void BrowserEventBus::timeoutPromise(size_t promise_id) {
    std::lock_guard<std::mutex> lock(promises_mutex_);
    
    auto it = event_promises_.find(promise_id);
    if (it != event_promises_.end() && !it->second->isCompleted()) {
        it->second->reject("Event timeout");
        event_promises_.erase(it);
    }
}

// ========== BrowserStateManager Implementation ==========

BrowserStateManager::BrowserStateManager(std::shared_ptr<BrowserEventBus> bus) 
    : current_state_(BrowserState::UNINITIALIZED), event_bus_(bus) {
    
    state_timestamps_[BrowserState::UNINITIALIZED] = std::chrono::steady_clock::now();
}

BrowserState BrowserStateManager::getCurrentState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_state_;
}

void BrowserStateManager::transitionToState(BrowserState new_state) {
    BrowserState old_state;
    
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (current_state_ == new_state) {
            return; // No change
        }
        
        if (!isValidStateTransition(current_state_, new_state)) {
            // Log warning but allow transition
            // TODO: Add proper logging
        }
        
        old_state = current_state_;
        current_state_ = new_state;
        state_timestamps_[new_state] = std::chrono::steady_clock::now();
    }
    
    // Notify state change
    notifyStateChange(old_state, new_state);
    
    // Emit event
    if (event_bus_) {
        event_bus_->emit(EventType::BROWSER_READY, stateToString(new_state));
    }
}

bool BrowserStateManager::isAtLeastState(BrowserState minimum_state) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return static_cast<int>(current_state_) >= static_cast<int>(minimum_state);
}

std::future<bool> BrowserStateManager::waitForState(BrowserState target_state, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if already in target state
    if (getCurrentState() == target_state) {
        promise->set_value(true);
        return future;
    }
    
    // Set up state change callback
    onStateChange(target_state, [promise]() {
        promise->set_value(true);
    });
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            try {
                promise->set_value(false);
            } catch (const std::future_error&) {
                // Promise already set
            }
        }).detach();
    }
    
    return future;
}

std::future<bool> BrowserStateManager::waitForMinimumState(BrowserState minimum_state, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if already at minimum state
    if (isAtLeastState(minimum_state)) {
        promise->set_value(true);
        return future;
    }
    
    // Subscribe to state changes until we reach minimum state
    if (event_bus_) {
        auto subscription_id = event_bus_->subscribe(EventType::BROWSER_READY, 
            [this, minimum_state, promise](const Event& event) {
                if (isAtLeastState(minimum_state)) {
                    try {
                        promise->set_value(true);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            });
    }
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            try {
                promise->set_value(false);
            } catch (const std::future_error&) {
                // Promise already set
            }
        }).detach();
    }
    
    return future;
}

void BrowserStateManager::onStateChange(BrowserState state, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    state_callbacks_[state].push_back(std::move(callback));
}

void BrowserStateManager::clearStateCallbacks(BrowserState state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    state_callbacks_[state].clear();
}

std::string BrowserStateManager::stateToString(BrowserState state) const {
    static const std::map<BrowserState, std::string> state_names = {
        {BrowserState::UNINITIALIZED, "UNINITIALIZED"},
        {BrowserState::LOADING, "LOADING"},
        {BrowserState::DOM_LOADING, "DOM_LOADING"},
        {BrowserState::DOM_READY, "DOM_READY"},
        {BrowserState::RESOURCES_LOADING, "RESOURCES_LOADING"},
        {BrowserState::RESOURCES_LOADED, "RESOURCES_LOADED"},
        {BrowserState::INTERACTIVE, "INTERACTIVE"},
        {BrowserState::JAVASCRIPT_EXECUTING, "JAVASCRIPT_EXECUTING"},
        {BrowserState::JAVASCRIPT_READY, "JAVASCRIPT_READY"},
        {BrowserState::FONTS_LOADING, "FONTS_LOADING"},
        {BrowserState::FONTS_LOADED, "FONTS_LOADED"},
        {BrowserState::IMAGES_LOADING, "IMAGES_LOADING"},
        {BrowserState::IMAGES_LOADED, "IMAGES_LOADED"},
        {BrowserState::STYLES_APPLYING, "STYLES_APPLYING"},
        {BrowserState::STYLES_APPLIED, "STYLES_APPLIED"},
        {BrowserState::FULLY_READY, "FULLY_READY"},
        {BrowserState::FRAMEWORK_READY, "FRAMEWORK_READY"},
        {BrowserState::ERROR_STATE, "ERROR_STATE"}
    };
    
    auto it = state_names.find(state);
    return it != state_names.end() ? it->second : "UNKNOWN";
}

std::chrono::milliseconds BrowserStateManager::getTimeInState(BrowserState state) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = state_timestamps_.find(state);
    if (it != state_timestamps_.end()) {
        auto end_time = (state == current_state_) ? 
            std::chrono::steady_clock::now() : 
            state_timestamps_.at(static_cast<BrowserState>(static_cast<int>(state) + 1));
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - it->second);
    }
    
    return std::chrono::milliseconds(0);
}

bool BrowserStateManager::hasBeenInState(BrowserState state) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_timestamps_.find(state) != state_timestamps_.end();
}

void BrowserStateManager::notifyStateChange(BrowserState old_state, BrowserState new_state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = state_callbacks_.find(new_state);
    if (it != state_callbacks_.end()) {
        for (auto& callback : it->second) {
            try {
                callback();
            } catch (const std::exception& e) {
                // Log error but continue
                // TODO: Add proper logging
            }
        }
    }
}

bool BrowserStateManager::isValidStateTransition(BrowserState from, BrowserState to) const {
    // Define valid state transitions
    // Most transitions are allowed, but some combinations don't make sense
    
    if (from == BrowserState::ERROR_STATE && to != BrowserState::LOADING) {
        return false; // Can only recover from error by starting fresh
    }
    
    // Generally allow progression forward or reset to loading
    int from_level = static_cast<int>(from);
    int to_level = static_cast<int>(to);
    
    return to_level >= from_level || to == BrowserState::LOADING || to == BrowserState::ERROR_STATE;
}

// ========== NetworkEventTracker Implementation ==========

NetworkEventTracker::NetworkEventTracker(std::shared_ptr<BrowserEventBus> bus) 
    : event_bus_(bus), active_request_count_(0) {
    
    last_request_time_ = std::chrono::steady_clock::now();
}

void NetworkEventTracker::onRequestStart(const std::string& url, const std::string& method) {
    {
        std::lock_guard<std::mutex> lock(requests_mutex_);
        active_requests_[url] = NetworkEvent(EventType::NETWORK_REQUEST_STARTED, url, 0, method, false);
        last_request_time_ = std::chrono::steady_clock::now();
    }
    
    active_request_count_++;
    
    if (event_bus_) {
        event_bus_->emit(NetworkEvent(EventType::NETWORK_REQUEST_STARTED, url, 0, method, false));
    }
}

void NetworkEventTracker::onRequestComplete(const std::string& url, int status_code, bool success) {
    {
        std::lock_guard<std::mutex> lock(requests_mutex_);
        active_requests_.erase(url);
    }
    
    active_request_count_--;
    
    if (event_bus_) {
        auto event_type = success ? EventType::NETWORK_REQUEST_COMPLETED : EventType::NETWORK_REQUEST_FAILED;
        event_bus_->emit(NetworkEvent(event_type, url, status_code, "GET", success));
    }
    
    // Check if network is now idle
    checkNetworkIdle();
}

void NetworkEventTracker::onRequestFailed(const std::string& url, const std::string& error) {
    onRequestComplete(url, 0, false);
}

bool NetworkEventTracker::isNetworkIdle(int idle_time_ms) const {
    if (active_request_count_.load() > 0) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto idle_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_request_time_);
    
    return idle_duration.count() >= idle_time_ms;
}

int NetworkEventTracker::getActiveRequestCount() const {
    return active_request_count_.load();
}

std::vector<std::string> NetworkEventTracker::getActiveRequests() const {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    std::vector<std::string> urls;
    urls.reserve(active_requests_.size());
    
    for (const auto& [url, event] : active_requests_) {
        urls.push_back(url);
    }
    
    return urls;
}

std::future<NetworkEvent> NetworkEventTracker::waitForRequest(const std::string& url_pattern, int timeout_ms) {
    auto promise = std::make_shared<EventPromise<NetworkEvent>>();
    
    if (event_bus_) {
        event_bus_->subscribe(EventType::NETWORK_REQUEST_COMPLETED,
            [promise, url_pattern, this](const Event& event) {
                if (matchesPattern(event.target, url_pattern)) {
                    if (auto net_event = dynamic_cast<const NetworkEvent*>(&event)) {
                        promise->resolve(*net_event);
                    }
                }
            });
    }
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Network request timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::future<bool> NetworkEventTracker::waitForNetworkIdle(int idle_time_ms, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if already idle
    if (isNetworkIdle(idle_time_ms)) {
        promise->set_value(true);
        return future;
    }
    
    // Set up monitoring for network idle
    std::thread([this, promise, idle_time_ms, timeout_ms]() {
        auto start_time = std::chrono::steady_clock::now();
        
        while (true) {
            if (isNetworkIdle(idle_time_ms)) {
                try {
                    promise->set_value(true);
                } catch (const std::future_error&) {
                    // Promise already set
                }
                break;
            }
            
            // Check timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);
            
            if (timeout_ms > 0 && elapsed.count() >= timeout_ms) {
                try {
                    promise->set_value(false);
                } catch (const std::future_error&) {
                    // Promise already set
                }
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }).detach();
    
    return future;
}

std::future<bool> NetworkEventTracker::waitForAllRequests(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if no active requests
    if (getActiveRequestCount() == 0) {
        promise->set_value(true);
        return future;
    }
    
    // Monitor until all requests complete
    std::thread([this, promise, timeout_ms]() {
        auto start_time = std::chrono::steady_clock::now();
        
        while (getActiveRequestCount() > 0) {
            // Check timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);
            
            if (timeout_ms > 0 && elapsed.count() >= timeout_ms) {
                try {
                    promise->set_value(false);
                } catch (const std::future_error&) {
                    // Promise already set
                }
                return;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        try {
            promise->set_value(true);
        } catch (const std::future_error&) {
            // Promise already set
        }
    }).detach();
    
    return future;
}

void NetworkEventTracker::checkNetworkIdle() {
    if (active_request_count_.load() == 0 && event_bus_) {
        // Emit network idle event after a short delay to ensure stability
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (active_request_count_.load() == 0) {
                event_bus_->emit(EventType::NETWORK_IDLE);
            }
        }).detach();
    }
}

bool NetworkEventTracker::matchesPattern(const std::string& url, const std::string& pattern) const {
    try {
        std::regex regex_pattern(pattern);
        return std::regex_search(url, regex_pattern);
    } catch (const std::exception&) {
        // If regex fails, try simple string matching
        return url.find(pattern) != std::string::npos;
    }
}

} // namespace BrowserEvents