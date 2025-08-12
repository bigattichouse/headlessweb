#include "BrowserEventBus.h"
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>

namespace BrowserEvents {

// ========== MutationTracker Implementation ==========

size_t MutationTracker::observeElement(const std::string& selector, const std::string& mutation_types) {
    static size_t next_observer_id = 1;
    size_t observer_id = next_observer_id++;
    
    // Stop any existing observer for this selector
    stopObserving(selector);
    
    std::string script = generateObserverScript(selector, mutation_types, observer_id);
    
    // TODO: Execute the JavaScript to set up the MutationObserver
    // This requires integration with the Browser class's JavaScript execution
    // For now, we'll store the observer info and set it up when integrated
    
    active_observers_[selector] = observer_id;
    return observer_id;
}

size_t MutationTracker::observeSubtree(const std::string& selector, const std::string& mutation_types) {
    return observeElement(selector, mutation_types + ",subtree");
}

void MutationTracker::stopObserving(const std::string& selector) {
    auto it = active_observers_.find(selector);
    if (it != active_observers_.end()) {
        // TODO: Execute JavaScript to disconnect the observer
        // window.hweb_mutation_observers[observer_id].disconnect();
        active_observers_.erase(it);
    }
}

void MutationTracker::stopAllObservers() {
    // TODO: Execute JavaScript to disconnect all observers
    // for (let observer of Object.values(window.hweb_mutation_observers)) {
    //     observer.disconnect();
    // }
    // window.hweb_mutation_observers = {};
    
    active_observers_.clear();
}

std::future<DOMEvent> MutationTracker::waitForElementAdd(const std::string& selector, int timeout_ms) {
    // Set up observer first
    observeElement(selector, "childList");
    
    // Wait for DOM_MUTATION event with "added" type
    auto promise = std::make_shared<EventPromise<DOMEvent>>();
    
    if (event_bus_) {
        event_bus_->subscribeOnce(EventType::DOM_MUTATION,
            [promise](const Event& event) {
                if (auto dom_event = dynamic_cast<const DOMEvent*>(&event)) {
                    if (dom_event->mutation_type == "added") {
                        promise->resolve(*dom_event);
                    }
                }
            },
            [selector](const Event& event) {
                return event.target == selector;
            });
    }
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Element add timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::future<DOMEvent> MutationTracker::waitForElementRemove(const std::string& selector, int timeout_ms) {
    // Set up observer first
    observeElement(selector, "childList");
    
    // Wait for DOM_MUTATION event with "removed" type
    auto promise = std::make_shared<EventPromise<DOMEvent>>();
    
    if (event_bus_) {
        event_bus_->subscribeOnce(EventType::DOM_MUTATION,
            [promise](const Event& event) {
                if (auto dom_event = dynamic_cast<const DOMEvent*>(&event)) {
                    if (dom_event->mutation_type == "removed") {
                        promise->resolve(*dom_event);
                    }
                }
            },
            [selector](const Event& event) {
                return event.target == selector;
            });
    }
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Element remove timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::future<DOMEvent> MutationTracker::waitForAttributeChange(const std::string& selector, const std::string& attribute, int timeout_ms) {
    // Set up observer first
    observeElement(selector, "attributes");
    
    // Wait for DOM_MUTATION event with "attributes" type
    auto promise = std::make_shared<EventPromise<DOMEvent>>();
    
    if (event_bus_) {
        event_bus_->subscribeOnce(EventType::DOM_MUTATION,
            [promise, attribute](const Event& event) {
                if (auto dom_event = dynamic_cast<const DOMEvent*>(&event)) {
                    if (dom_event->mutation_type == "attributes" && 
                        (attribute.empty() || dom_event->data == attribute)) {
                        promise->resolve(*dom_event);
                    }
                }
            },
            [selector](const Event& event) {
                return event.target == selector;
            });
    }
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Attribute change timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::future<DOMEvent> MutationTracker::waitForTextChange(const std::string& selector, int timeout_ms) {
    // Set up observer first
    observeElement(selector, "characterData,childList,subtree");
    
    // Wait for DOM_MUTATION event with "characterData" type
    auto promise = std::make_shared<EventPromise<DOMEvent>>();
    
    if (event_bus_) {
        event_bus_->subscribeOnce(EventType::DOM_MUTATION,
            [promise](const Event& event) {
                if (auto dom_event = dynamic_cast<const DOMEvent*>(&event)) {
                    if (dom_event->mutation_type == "characterData" || 
                        dom_event->mutation_type == "childList") {
                        promise->resolve(*dom_event);
                    }
                }
            },
            [selector](const Event& event) {
                return event.target == selector;
            });
    }
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Text change timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::string MutationTracker::generateObserverScript(const std::string& selector, const std::string& mutation_types, size_t observer_id) {
    std::ostringstream script;
    
    // Initialize global mutation observer storage if needed
    script << R"JS(
if (typeof window.hweb_mutation_observers === 'undefined') {
    window.hweb_mutation_observers = {};
}

// Parse mutation types
let observerConfig = {
    childList: false,
    attributes: false,
    characterData: false,
    subtree: false,
    attributeOldValue: false,
    characterDataOldValue: false
};
)JS";
    
    // Parse mutation_types string and set config
    std::istringstream types_stream(mutation_types);
    std::string type;
    while (std::getline(types_stream, type, ',')) {
        // Trim whitespace
        type.erase(0, type.find_first_not_of(" \t"));
        type.erase(type.find_last_not_of(" \t") + 1);
        
        script << "observerConfig." << type << " = true;\n";
    }
    
    // Create the observer
    script << R"JS(
// Find target element
let targetElement = document.querySelector(')JS" << selector << R"JS(');
if (!targetElement) {
    console.warn('MutationObserver target not found: )JS" << selector << R"JS(');
    false; // Return false to indicate failure
} else {
    // Create observer callback
    let callback = function(mutations) {
        mutations.forEach(function(mutation) {
            // Emit event through HeadlessWeb event system
            if (typeof window.hweb_emit_event === 'function') {
                window.hweb_emit_event({
                    type: 'DOM_MUTATION',
                    target: ')JS" << selector << R"JS(',
                    mutation_type: mutation.type,
                    observer_id: )JS" << observer_id << R"JS(,
                    added_nodes: mutation.addedNodes.length,
                    removed_nodes: mutation.removedNodes.length,
                    attribute_name: mutation.attributeName,
                    old_value: mutation.oldValue
                });
            }
        });
    };
    
    // Create and start observer
    let observer = new MutationObserver(callback);
    observer.observe(targetElement, observerConfig);
    
    // Store observer for later cleanup  
)JS";
    script << "    window.hweb_mutation_observers[" << observer_id << "] = observer;\n";
    script << R"JS(    
    return true; // Return true to indicate success
}
)JS";
    
    return script.str();
}

} // namespace BrowserEvents