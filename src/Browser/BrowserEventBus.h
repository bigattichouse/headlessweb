#pragma once

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <future>
#include <chrono>

namespace BrowserEvents {

// Event types for the browser event bus
enum class EventType {
    // Navigation events
    NAVIGATION_STARTED,
    NAVIGATION_COMPLETED,
    NAVIGATION_FAILED,
    URL_CHANGED,
    TITLE_CHANGED,
    
    // DOM events
    DOM_READY,
    DOM_CONTENT_LOADED,
    DOM_MUTATION,
    ELEMENT_READY,
    ELEMENT_VISIBLE,
    ELEMENT_HIDDEN,
    
    // Network events
    NETWORK_REQUEST_STARTED,
    NETWORK_REQUEST_COMPLETED,
    NETWORK_REQUEST_FAILED,
    NETWORK_IDLE,
    
    // Resource events
    RESOURCE_LOADED,
    ALL_RESOURCES_LOADED,
    
    // Form events
    FORM_FIELD_CHANGED,
    FORM_SUBMITTED,
    INPUT_COMPLETED,
    
    // Framework events
    FRAMEWORK_READY,
    SPA_NAVIGATION,
    
    // Browser state events
    BROWSER_READY,
    PAGE_INTERACTIVE,
    PAGE_COMPLETE,
    
    // Session events
    SESSION_RESTORED,
    COOKIES_SET,
    STORAGE_UPDATED
};

// Base event class - made polymorphic for dynamic_cast
struct Event {
    EventType type;
    std::string target;  // selector, URL, or identifier
    std::string data;    // additional event data
    std::chrono::steady_clock::time_point timestamp;
    
    Event(EventType t, const std::string& tgt = "", const std::string& d = "")
        : type(t), target(tgt), data(d), timestamp(std::chrono::steady_clock::now()) {}
    
    virtual ~Event() = default;  // Make polymorphic
};

// Specialized event types
struct NavigationEvent : public Event {
    std::string url;
    std::string previous_url;
    bool success;
    
    NavigationEvent(EventType t, const std::string& current_url, const std::string& prev_url = "", bool s = true)
        : Event(t), url(current_url), previous_url(prev_url), success(s) {}
};

struct DOMEvent : public Event {
    std::string selector;
    std::string mutation_type;  // "added", "removed", "attributes", "characterData"
    
    DOMEvent(EventType t, const std::string& sel, const std::string& mutation = "")
        : Event(t, sel), selector(sel), mutation_type(mutation) {}
};

struct NetworkEvent : public Event {
    std::string url;
    int status_code;
    std::string method;
    bool completed;
    
    // Default constructor for container usage
    NetworkEvent() : Event(EventType::NETWORK_REQUEST_STARTED), status_code(0), method("GET"), completed(false) {}
    
    NetworkEvent(EventType t, const std::string& request_url, int status = 0, const std::string& m = "GET", bool c = false)
        : Event(t, request_url), url(request_url), status_code(status), method(m), completed(c) {}
};

// Event handler function types
using EventHandler = std::function<void(const Event&)>;
using EventCondition = std::function<bool(const Event&)>;

// Promise-based event waiting
template<typename T = Event>
class EventPromise {
private:
    std::promise<T> promise_;
    std::future<T> future_;
    std::atomic<bool> completed_;
    
public:
    EventPromise() : future_(promise_.get_future()), completed_(false) {}
    
    void resolve(const T& event) {
        if (!completed_.exchange(true)) {
            promise_.set_value(event);
        }
    }
    
    void reject(const std::string& error) {
        if (!completed_.exchange(true)) {
            promise_.set_exception(std::make_exception_ptr(std::runtime_error(error)));
        }
    }
    
    std::future<T>& getFuture() { return future_; }
    bool isCompleted() const { return completed_.load(); }
};

// Unified event bus for all browser events
class BrowserEventBus {
private:
    struct Subscription {
        size_t id;
        EventType type;
        EventHandler handler;
        EventCondition condition;  // Optional condition for filtering
        bool once;  // One-time subscription
        
        Subscription(size_t i, EventType t, EventHandler h, EventCondition c = nullptr, bool o = false)
            : id(i), type(t), handler(std::move(h)), condition(std::move(c)), once(o) {}
    };
    
    mutable std::mutex subscriptions_mutex_;
    std::map<EventType, std::vector<std::shared_ptr<Subscription>>> subscriptions_;
    std::atomic<size_t> next_subscription_id_;
    
    // Promise-based waiting infrastructure
    mutable std::mutex promises_mutex_;
    std::map<size_t, std::shared_ptr<EventPromise<Event>>> event_promises_;
    std::atomic<size_t> next_promise_id_;

public:
    BrowserEventBus() : next_subscription_id_(1), next_promise_id_(1) {}
    ~BrowserEventBus() = default;
    
    // Subscription management
    size_t subscribe(EventType type, EventHandler handler, EventCondition condition = nullptr);
    size_t subscribeOnce(EventType type, EventHandler handler, EventCondition condition = nullptr);
    void unsubscribe(size_t subscription_id);
    void unsubscribeAll(EventType type);
    void clearAllSubscriptions();
    
    // Event emission
    void emit(const Event& event);
    void emit(EventType type, const std::string& target = "", const std::string& data = "");
    
    // Promise-based waiting
    std::future<Event> waitForEvent(EventType type, int timeout_ms = 5000, EventCondition condition = nullptr);
    std::future<NavigationEvent> waitForNavigation(int timeout_ms = 10000, const std::string& expected_url = "");
    std::future<DOMEvent> waitForDOMChange(const std::string& selector, int timeout_ms = 5000);
    std::future<NetworkEvent> waitForNetworkIdle(int idle_time_ms = 500, int timeout_ms = 10000);
    
    // Utility methods
    bool hasSubscriptions(EventType type) const;
    size_t getSubscriptionCount(EventType type) const;
    void setEventTimeout(size_t promise_id, int timeout_ms);
    
private:
    void cleanupCompletedPromises();
    void timeoutPromise(size_t promise_id);
};

// Browser state management
enum class BrowserState {
    UNINITIALIZED,
    LOADING,
    DOM_LOADING,
    DOM_READY,
    RESOURCES_LOADING,
    RESOURCES_LOADED,
    INTERACTIVE,
    FULLY_READY,
    FRAMEWORK_READY,
    ERROR_STATE
};

class BrowserStateManager {
private:
    mutable std::mutex state_mutex_;
    BrowserState current_state_;
    std::map<BrowserState, std::chrono::steady_clock::time_point> state_timestamps_;
    
    // State change notifications
    std::shared_ptr<BrowserEventBus> event_bus_;
    std::map<BrowserState, std::vector<std::function<void()>>> state_callbacks_;

public:
    explicit BrowserStateManager(std::shared_ptr<BrowserEventBus> bus);
    
    // State management
    BrowserState getCurrentState() const;
    void transitionToState(BrowserState new_state);
    bool isAtLeastState(BrowserState minimum_state) const;
    
    // State waiting
    std::future<bool> waitForState(BrowserState target_state, int timeout_ms = 10000);
    std::future<bool> waitForMinimumState(BrowserState minimum_state, int timeout_ms = 10000);
    
    // State callbacks
    void onStateChange(BrowserState state, std::function<void()> callback);
    void clearStateCallbacks(BrowserState state);
    
    // Utility methods
    std::string stateToString(BrowserState state) const;
    std::chrono::milliseconds getTimeInState(BrowserState state) const;
    bool hasBeenInState(BrowserState state) const;
    
private:
    void notifyStateChange(BrowserState old_state, BrowserState new_state);
    bool isValidStateTransition(BrowserState from, BrowserState to) const;
};

// MutationObserver integration for DOM changes
class MutationTracker {
private:
    std::shared_ptr<BrowserEventBus> event_bus_;
    std::map<std::string, size_t> active_observers_;  // selector -> observer_id
    
public:
    explicit MutationTracker(std::shared_ptr<BrowserEventBus> bus) : event_bus_(bus) {}
    
    // Observer management
    size_t observeElement(const std::string& selector, const std::string& mutation_types = "childList,attributes,characterData");
    size_t observeSubtree(const std::string& selector, const std::string& mutation_types = "childList,subtree");
    void stopObserving(const std::string& selector);
    void stopAllObservers();
    
    // Convenience methods
    std::future<DOMEvent> waitForElementAdd(const std::string& selector, int timeout_ms = 5000);
    std::future<DOMEvent> waitForElementRemove(const std::string& selector, int timeout_ms = 5000);
    std::future<DOMEvent> waitForAttributeChange(const std::string& selector, const std::string& attribute, int timeout_ms = 5000);
    std::future<DOMEvent> waitForTextChange(const std::string& selector, int timeout_ms = 5000);
    
private:
    std::string generateObserverScript(const std::string& selector, const std::string& mutation_types, size_t observer_id);
};

// Network request tracking
class NetworkEventTracker {
private:
    std::shared_ptr<BrowserEventBus> event_bus_;
    std::map<std::string, NetworkEvent> active_requests_;
    mutable std::mutex requests_mutex_;
    
    // Network idle detection
    std::chrono::steady_clock::time_point last_request_time_;
    std::atomic<int> active_request_count_;
    
public:
    explicit NetworkEventTracker(std::shared_ptr<BrowserEventBus> bus);
    
    // Request tracking
    void onRequestStart(const std::string& url, const std::string& method = "GET");
    void onRequestComplete(const std::string& url, int status_code, bool success = true);
    void onRequestFailed(const std::string& url, const std::string& error);
    
    // Network state
    bool isNetworkIdle(int idle_time_ms = 500) const;
    int getActiveRequestCount() const;
    std::vector<std::string> getActiveRequests() const;
    
    // Waiting methods
    std::future<NetworkEvent> waitForRequest(const std::string& url_pattern, int timeout_ms = 10000);
    std::future<bool> waitForNetworkIdle(int idle_time_ms = 500, int timeout_ms = 10000);
    std::future<bool> waitForAllRequests(int timeout_ms = 10000);
    
private:
    void checkNetworkIdle();
    bool matchesPattern(const std::string& url, const std::string& pattern) const;
};

} // namespace BrowserEvents