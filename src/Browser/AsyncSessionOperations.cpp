#include "BrowserEventBus.h"
#include <algorithm>
#include <thread>
#include <sstream>

namespace BrowserEvents {

// ========== AsyncSessionOperations Implementation ==========

std::future<bool> AsyncSessionOperations::waitForUserAgentSet(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for user agent set
    event_bus_->subscribeOnce(EventType::USER_AGENT_SET,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForViewportSet(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for viewport set
    event_bus_->subscribeOnce(EventType::VIEWPORT_SET,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForCookiesRestored(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for cookies restored
    event_bus_->subscribeOnce(EventType::COOKIES_RESTORED,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForStorageRestored(const std::string& storage_type, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    EventType event_type = storage_type == "localStorage" ? EventType::LOCAL_STORAGE_RESTORED : EventType::SESSION_STORAGE_RESTORED;
    
    // Set up event listener for storage restored
    event_bus_->subscribeOnce(event_type,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForFormStateRestored(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for form state restored
    event_bus_->subscribeOnce(EventType::FORM_STATE_RESTORED,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForActiveElementsRestored(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for active elements restored
    event_bus_->subscribeOnce(EventType::ACTIVE_ELEMENTS_RESTORED,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForCustomAttributesRestored(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for custom attributes restored
    event_bus_->subscribeOnce(EventType::CUSTOM_ATTRIBUTES_RESTORED,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForCustomStateRestored(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for custom state restored
    event_bus_->subscribeOnce(EventType::CUSTOM_STATE_RESTORED,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForScrollPositionsRestored(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for scroll positions restored
    event_bus_->subscribeOnce(EventType::SCROLL_POSITIONS_RESTORED,
        [promise](const Event& event) {
            try {
                promise->set_value(true);
            } catch (const std::future_error&) {
                // Promise already set
            }
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

std::future<bool> AsyncSessionOperations::waitForSessionRestorationComplete(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for session restoration complete
    event_bus_->subscribeOnce(EventType::SESSION_RESTORATION_COMPLETE,
        [promise](const Event& event) {
            if (auto session_event = dynamic_cast<const SessionEvent*>(&event)) {
                try {
                    promise->set_value(session_event->success);
                } catch (const std::future_error&) {
                    // Promise already set
                }
            } else {
                try {
                    promise->set_value(true);
                } catch (const std::future_error&) {
                    // Promise already set
                }
            }
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

// Sequential restoration chain
std::future<bool> AsyncSessionOperations::restoreSessionAsync(const std::string& session_name, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // This is a coordination method - the actual restoration steps will be performed
    // by the Browser class, and this method waits for the complete restoration chain
    
    // Set up event listener for session restoration complete
    event_bus_->subscribeOnce(EventType::SESSION_RESTORATION_COMPLETE,
        [promise, session_name](const Event& event) {
            if (auto session_event = dynamic_cast<const SessionEvent*>(&event)) {
                // Check if this is the session we're waiting for
                if (session_event->session_name == session_name || session_name.empty()) {
                    try {
                        promise->set_value(session_event->success);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            } else {
                try {
                    promise->set_value(true);
                } catch (const std::future_error&) {
                    // Promise already set
                }
            }
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

// Session event emission methods
void AsyncSessionOperations::emitUserAgentSet(const std::string& user_agent) {
    emitSessionEvent(EventType::USER_AGENT_SET, "", "user_agent", user_agent, 1, 1, true);
}

void AsyncSessionOperations::emitViewportSet(int width, int height) {
    std::string viewport_data = std::to_string(width) + "x" + std::to_string(height);
    emitSessionEvent(EventType::VIEWPORT_SET, "", "viewport", viewport_data, 1, 1, true);
}

void AsyncSessionOperations::emitCookiesRestored(int count) {
    emitSessionEvent(EventType::COOKIES_RESTORED, "", "cookies", "", count, count, true);
}

void AsyncSessionOperations::emitStorageRestored(const std::string& storage_type, int items) {
    EventType event_type = storage_type == "localStorage" ? EventType::LOCAL_STORAGE_RESTORED : EventType::SESSION_STORAGE_RESTORED;
    emitSessionEvent(event_type, "", "storage", storage_type, items, items, true);
}

void AsyncSessionOperations::emitFormStateRestored(int fields) {
    emitSessionEvent(EventType::FORM_STATE_RESTORED, "", "form_state", "", fields, fields, true);
}

void AsyncSessionOperations::emitActiveElementsRestored(int elements) {
    emitSessionEvent(EventType::ACTIVE_ELEMENTS_RESTORED, "", "active_elements", "", elements, elements, true);
}

void AsyncSessionOperations::emitCustomAttributesRestored(int attributes) {
    emitSessionEvent(EventType::CUSTOM_ATTRIBUTES_RESTORED, "", "custom_attributes", "", attributes, attributes, true);
}

void AsyncSessionOperations::emitCustomStateRestored(int states) {
    emitSessionEvent(EventType::CUSTOM_STATE_RESTORED, "", "custom_state", "", states, states, true);
}

void AsyncSessionOperations::emitScrollPositionsRestored(int positions) {
    emitSessionEvent(EventType::SCROLL_POSITIONS_RESTORED, "", "scroll_positions", "", positions, positions, true);
}

void AsyncSessionOperations::emitSessionRestorationComplete(const std::string& session_name, bool success) {
    emitSessionEvent(EventType::SESSION_RESTORATION_COMPLETE, session_name, "complete", "", 1, 1, success);
}

// Private method
void AsyncSessionOperations::emitSessionEvent(EventType type, const std::string& session_name, const std::string& operation,
                                            const std::string& component, int processed, int total, bool success) {
    if (event_bus_) {
        SessionEvent event(type, session_name, operation, component, processed, total, success);
        event_bus_->emit(event);
    }
}

} // namespace BrowserEvents