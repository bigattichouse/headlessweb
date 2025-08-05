#include "BrowserEventBus.h"
#include <algorithm>
#include <thread>
#include <sstream>
#include <random>

namespace BrowserEvents {

// ========== AsyncDOMOperations Implementation ==========

std::future<bool> AsyncDOMOperations::fillInputAsync(const std::string& selector, const std::string& value, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    std::string operation_id = generateOperationId();
    
    // Set up event listener for input completion
    event_bus_->subscribeOnce(EventType::INPUT_FILLED,
        [promise, selector](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                if (dom_event->selector == selector) {
                    try {
                        promise->set_value(dom_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [selector](const Event& event) {
            return event.target == selector;
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
    
    return std::move(future);
}

std::future<bool> AsyncDOMOperations::clickElementAsync(const std::string& selector, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    std::string operation_id = generateOperationId();
    
    // Set up event listener for click completion
    event_bus_->subscribeOnce(EventType::ELEMENT_CLICKED,
        [promise, selector](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                if (dom_event->selector == selector) {
                    try {
                        promise->set_value(dom_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [selector](const Event& event) {
            return event.target == selector;
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
    
    return std::move(future);
}

std::future<bool> AsyncDOMOperations::selectOptionAsync(const std::string& selector, const std::string& value, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    std::string operation_id = generateOperationId();
    
    // Set up event listener for selection completion
    event_bus_->subscribeOnce(EventType::OPTION_SELECTED,
        [promise, selector, value](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                if (dom_event->selector == selector && dom_event->value == value) {
                    try {
                        promise->set_value(dom_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [selector](const Event& event) {
            return event.target == selector;
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
    
    return std::move(future);
}

std::future<bool> AsyncDOMOperations::submitFormAsync(const std::string& selector, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for form submission completion
    event_bus_->subscribeOnce(EventType::FORM_SUBMITTED,
        [promise, selector](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                if (dom_event->selector == selector) {
                    try {
                        promise->set_value(dom_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [selector](const Event& event) {
            return event.target == selector;
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
    
    return std::move(future);
}

std::future<bool> AsyncDOMOperations::checkElementAsync(const std::string& selector, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for checkbox change completion
    event_bus_->subscribeOnce(EventType::CHECKBOX_CHANGED,
        [promise, selector](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                if (dom_event->selector == selector && dom_event->value == "checked") {
                    try {
                        promise->set_value(dom_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [selector](const Event& event) {
            return event.target == selector;
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
    
    return std::move(future);
}

std::future<bool> AsyncDOMOperations::uncheckElementAsync(const std::string& selector, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for checkbox uncheck completion
    event_bus_->subscribeOnce(EventType::CHECKBOX_CHANGED,
        [promise, selector](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                if (dom_event->selector == selector && dom_event->value == "unchecked") {
                    try {
                        promise->set_value(dom_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [selector](const Event& event) {
            return event.target == selector;
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
    
    return std::move(future);
}

std::future<bool> AsyncDOMOperations::focusElementAsync(const std::string& selector, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for focus completion
    event_bus_->subscribeOnce(EventType::INPUT_FOCUSED,
        [promise, selector](const Event& event) {
            if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                if (dom_event->selector == selector) {
                    try {
                        promise->set_value(dom_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [selector](const Event& event) {
            return event.target == selector;
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
    
    return std::move(future);
}

std::future<DOMInteractionEvent> AsyncDOMOperations::waitForInputEvent(const std::string& selector, const std::string& event_type, int timeout_ms) {
    auto promise = std::make_shared<EventPromise<DOMInteractionEvent>>();
    
    EventType event_enum = EventType::INPUT_CHANGED;
    if (event_type == "input") event_enum = EventType::INPUT_CHANGED;
    else if (event_type == "focus") event_enum = EventType::INPUT_FOCUSED;
    else if (event_type == "blur") event_enum = EventType::INPUT_BLURRED;
    else if (event_type == "change") event_enum = EventType::INPUT_CHANGED;
    
    if (event_bus_) {
        event_bus_->subscribeOnce(event_enum,
            [promise, event_enum](const Event& event) {
                if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                    promise->resolve(*dom_event);
                } else {
                    // Convert generic event to DOMInteractionEvent
                    DOMInteractionEvent converted_event(event_enum, event.target, "generic", "", true);
                    promise->resolve(converted_event);
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
            promise->reject("Input event timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

std::future<DOMInteractionEvent> AsyncDOMOperations::waitForElementEvent(const std::string& selector, const std::string& event_type, int timeout_ms) {
    auto promise = std::make_shared<EventPromise<DOMInteractionEvent>>();
    
    EventType event_enum = EventType::ELEMENT_CLICKED;
    if (event_type == "click") event_enum = EventType::ELEMENT_CLICKED;
    else if (event_type == "select") event_enum = EventType::ELEMENT_SELECTED;
    else if (event_type == "submit") event_enum = EventType::FORM_SUBMITTED;
    
    if (event_bus_) {
        event_bus_->subscribeOnce(event_enum,
            [promise, event_enum](const Event& event) {
                if (auto dom_event = dynamic_cast<const DOMInteractionEvent*>(&event)) {
                    promise->resolve(*dom_event);
                } else {
                    // Convert generic event to DOMInteractionEvent
                    DOMInteractionEvent converted_event(event_enum, event.target, "generic", "", true);
                    promise->resolve(converted_event);
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
            promise->reject("Element event timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

// JavaScript generation methods
std::string AsyncDOMOperations::generateEventListenerScript(const std::string& selector, const std::string& event_type, const std::string& operation_id) const {
    std::ostringstream script;
    
    script << R"JS(
(function(selector, eventType, operationId) {
    try {
        var element = document.querySelector(selector);
        if (!element) {
            window.hweb_dom_result = {
                operation_id: operationId,
                success: false,
                error: 'Element not found: ' + selector
            };
            return false;
        }
        
        // Set up event listener
        var eventHandler = function(event) {
            window.hweb_dom_result = {
                operation_id: operationId,
                success: true,
                selector: selector,
                event_type: eventType,
                value: element.value || '',
                timestamp: Date.now()
            };
            
            // Remove listener after firing
            element.removeEventListener(eventType, eventHandler);
        };
        
        element.addEventListener(eventType, eventHandler);
        return true;
        
    } catch(e) {
        window.hweb_dom_result = {
            operation_id: operationId,
            success: false,
            error: e.message
        };
        return false;
    }
})()JS" << "('" << selector << "', '" << event_type << "', '" << operation_id << "');";
    
    return script.str();
}

std::string AsyncDOMOperations::generateInputFillScript(const std::string& selector, const std::string& value, const std::string& operation_id) const {
    std::ostringstream script;
    
    // Escape quotes in value for JavaScript
    std::string escaped_value = value;
    size_t pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    script << R"JS(
(function(selector, value, operationId) {
    try {
        var element = document.querySelector(selector);
        if (!element) {
            window.hweb_dom_result = {
                operation_id: operationId,
                success: false,
                error: 'Element not found: ' + selector
            };
            return false;
        }
        
        // Focus and fill the input
        element.focus();
        element.click();
        element.value = '';
        element.value = value;
        
        // Dispatch comprehensive events for modern frameworks
        var events = ['focus', 'input', 'keydown', 'keyup', 'change'];
        events.forEach(function(eventType) {
            element.dispatchEvent(new Event(eventType, { bubbles: true }));
        });
        
        // For React/Vue compatibility
        if (element._valueTracker) {
            element._valueTracker.setValue(value);
        }
        
        // Set result and emit HeadlessWeb event
        window.hweb_dom_result = {
            operation_id: operationId,
            success: true,
            selector: selector,
            value: value,
            timestamp: Date.now()
        };
        
        // Emit custom HeadlessWeb event for async detection
        if (typeof window.hweb_emit_dom_event === 'function') {
            window.hweb_emit_dom_event('INPUT_FILLED', selector, value, true);
        }
        
        return true;
        
    } catch(e) {
        window.hweb_dom_result = {
            operation_id: operationId,
            success: false,
            error: e.message
        };
        return false;
    }
})()JS" << "('" << selector << "', '" << escaped_value << "', '" << operation_id << "');";
    
    return script.str();
}

std::string AsyncDOMOperations::generateClickScript(const std::string& selector, const std::string& operation_id) const {
    std::ostringstream script;
    
    script << R"JS(
(function(selector, operationId) {
    try {
        var element = document.querySelector(selector);
        if (!element) {
            window.hweb_dom_result = {
                operation_id: operationId,
                success: false,
                error: 'Element not found: ' + selector
            };
            return false;
        }
        
        // Perform click with full event simulation
        element.focus();
        element.click();
        
        // Dispatch mouse events for compatibility
        var mouseEvents = ['mousedown', 'mouseup', 'click'];
        mouseEvents.forEach(function(eventType) {
            element.dispatchEvent(new MouseEvent(eventType, { bubbles: true }));
        });
        
        // Set result and emit HeadlessWeb event
        window.hweb_dom_result = {
            operation_id: operationId,
            success: true,
            selector: selector,
            timestamp: Date.now()
        };
        
        // Emit custom HeadlessWeb event for async detection
        if (typeof window.hweb_emit_dom_event === 'function') {
            window.hweb_emit_dom_event('ELEMENT_CLICKED', selector, '', true);
        }
        
        return true;
        
    } catch(e) {
        window.hweb_dom_result = {
            operation_id: operationId,
            success: false,
            error: e.message
        };
        return false;
    }
})()JS" << "('" << selector << "', '" << operation_id << "');";
    
    return script.str();
}

std::string AsyncDOMOperations::generateSelectScript(const std::string& selector, const std::string& value, const std::string& operation_id) const {
    std::ostringstream script;
    
    // Escape quotes in value for JavaScript
    std::string escaped_value = value;
    size_t pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    script << R"JS(
(function(selector, value, operationId) {
    try {
        var element = document.querySelector(selector);
        if (!element) {
            window.hweb_dom_result = {
                operation_id: operationId,
                success: false,
                error: 'Element not found: ' + selector
            };
            return false;
        }
        
        // Focus the select element
        element.focus();
        
        // Set the value
        element.value = value;
        
        // Dispatch change event
        element.dispatchEvent(new Event('change', { bubbles: true }));
        element.dispatchEvent(new Event('input', { bubbles: true }));
        
        // Set result and emit HeadlessWeb event
        window.hweb_dom_result = {
            operation_id: operationId,
            success: true,
            selector: selector,
            value: value,
            timestamp: Date.now()
        };
        
        // Emit custom HeadlessWeb event for async detection
        if (typeof window.hweb_emit_dom_event === 'function') {
            window.hweb_emit_dom_event('OPTION_SELECTED', selector, value, true);
        }
        
        return true;
        
    } catch(e) {
        window.hweb_dom_result = {
            operation_id: operationId,
            success: false,
            error: e.message
        };
        return false;
    }
})()JS" << "('" << selector << "', '" << escaped_value << "', '" << operation_id << "');";
    
    return script.str();
}

// Private methods
void AsyncDOMOperations::emitDOMInteractionEvent(EventType type, const std::string& selector, const std::string& interaction, 
                                                const std::string& value, bool success) {
    if (event_bus_) {
        DOMInteractionEvent event(type, selector, interaction, value, success);
        event_bus_->emit(event);
    }
}

std::string AsyncDOMOperations::generateOperationId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    return "dom_op_" + std::to_string(dis(gen)) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

} // namespace BrowserEvents