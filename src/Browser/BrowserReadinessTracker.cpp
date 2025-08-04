#include "BrowserEventBus.h"
#include <algorithm>
#include <thread>
#include <sstream>

namespace BrowserEvents {

// ========== BrowserReadinessTracker Implementation ==========

BrowserReadinessTracker::BrowserReadinessTracker(std::shared_ptr<BrowserEventBus> bus) 
    : event_bus_(bus) {
    
    current_state_.last_change = std::chrono::steady_clock::now();
    setupEventSubscriptions();
    generateReadinessCheckScript();
}

void BrowserReadinessTracker::setReadinessConfig(const ReadinessConfig& config) {
    std::lock_guard<std::mutex> lock(readiness_mutex_);
    config_ = config;
}

bool BrowserReadinessTracker::isFullyReady() const {
    std::lock_guard<std::mutex> lock(readiness_mutex_);
    return current_state_.isFullyReady();
}

bool BrowserReadinessTracker::isBasicReady() const {
    std::lock_guard<std::mutex> lock(readiness_mutex_);
    return current_state_.isBasicReady();
}

bool BrowserReadinessTracker::isInteractive() const {
    std::lock_guard<std::mutex> lock(readiness_mutex_);
    return current_state_.isInteractive();
}

BrowserReadinessTracker::ReadinessState BrowserReadinessTracker::getCurrentState() const {
    std::lock_guard<std::mutex> lock(readiness_mutex_);
    return current_state_;
}

std::future<bool> BrowserReadinessTracker::waitForFullReadiness(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if already fully ready
    if (isFullyReady()) {
        promise->set_value(true);
        return std::move(future);
    }
    
    // Subscribe to readiness events
    if (event_bus_) {
        event_bus_->subscribe(EventType::BROWSER_READY,
            [this, promise](const Event& event) {
                if (isFullyReady()) {
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
    
    return std::move(future);
}

std::future<bool> BrowserReadinessTracker::waitForBasicReadiness(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if already basically ready
    if (isBasicReady()) {
        promise->set_value(true);
        return std::move(future);
    }
    
    // Subscribe to DOM and JavaScript readiness events
    if (event_bus_) {
        auto subscription_id = event_bus_->subscribe(EventType::JAVASCRIPT_READY,
            [this, promise](const Event& event) {
                if (isBasicReady()) {
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
    
    return std::move(future);
}

std::future<bool> BrowserReadinessTracker::waitForInteractive(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if already interactive
    if (isInteractive()) {
        promise->set_value(true);
        return std::move(future);
    }
    
    // Subscribe to DOM ready events
    if (event_bus_) {
        event_bus_->subscribe(EventType::DOM_READY,
            [this, promise](const Event& event) {
                if (isInteractive()) {
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
    
    return std::move(future);
}

std::future<bool> BrowserReadinessTracker::waitForJavaScriptReady(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if JavaScript is already ready
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (current_state_.javascript_ready) {
            promise->set_value(true);
            return std::move(future);
        }
    }
    
    // Subscribe to JavaScript ready events
    if (event_bus_) {
        event_bus_->subscribeOnce(EventType::JAVASCRIPT_READY,
            [promise](const Event& event) {
                try {
                    promise->set_value(true);
                } catch (const std::future_error&) {
                    // Promise already set
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
    
    return std::move(future);
}

std::future<bool> BrowserReadinessTracker::waitForResourcesLoaded(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if resources are already loaded
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (current_state_.resources_loaded) {
            promise->set_value(true);
            return std::move(future);
        }
    }
    
    // Subscribe to resources loaded events
    if (event_bus_) {
        event_bus_->subscribeOnce(EventType::RESOURCES_COMPLETE,
            [promise](const Event& event) {
                try {
                    promise->set_value(true);
                } catch (const std::future_error&) {
                    // Promise already set
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
    
    return std::move(future);
}

std::future<bool> BrowserReadinessTracker::waitForNetworkIdle(int idle_time_ms, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Check if network is already idle
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (current_state_.network_idle) {
            promise->set_value(true);
            return std::move(future);
        }
    }
    
    // Subscribe to network idle events
    if (event_bus_) {
        event_bus_->subscribeOnce(EventType::NETWORK_IDLE,
            [promise](const Event& event) {
                try {
                    promise->set_value(true);
                } catch (const std::future_error&) {
                    // Promise already set
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
    
    return std::move(future);
}

// Manual state updates
void BrowserReadinessTracker::updateDOMReady() {
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (!current_state_.dom_ready) {
            current_state_.dom_ready = true;
            current_state_.last_change = std::chrono::steady_clock::now();
        }
    }
    
    if (event_bus_) {
        event_bus_->emit(EventType::DOM_READY);
    }
    
    checkOverallReadiness();
}

void BrowserReadinessTracker::updateJavaScriptReady() {
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (!current_state_.javascript_ready) {
            current_state_.javascript_ready = true;
            current_state_.last_change = std::chrono::steady_clock::now();
        }
    }
    
    if (event_bus_) {
        event_bus_->emit(EventType::JAVASCRIPT_READY);
    }
    
    checkOverallReadiness();
}

void BrowserReadinessTracker::updateResourcesLoaded() {
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (!current_state_.resources_loaded) {
            current_state_.resources_loaded = true;
            current_state_.last_change = std::chrono::steady_clock::now();
        }
    }
    
    if (event_bus_) {
        event_bus_->emit(EventType::RESOURCES_COMPLETE);
    }
    
    checkOverallReadiness();
}

void BrowserReadinessTracker::updateFontsLoaded() {
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (!current_state_.fonts_loaded) {
            current_state_.fonts_loaded = true;
            current_state_.last_change = std::chrono::steady_clock::now();
        }
    }
    
    if (event_bus_) {
        event_bus_->emit(EventType::FONTS_LOADED);
    }
    
    checkOverallReadiness();
}

void BrowserReadinessTracker::updateImagesLoaded() {
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (!current_state_.images_loaded) {
            current_state_.images_loaded = true;
            current_state_.last_change = std::chrono::steady_clock::now();
        }
    }
    
    if (event_bus_) {
        event_bus_->emit(EventType::IMAGES_LOADED);
    }
    
    checkOverallReadiness();
}

void BrowserReadinessTracker::updateStylesApplied() {
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (!current_state_.styles_applied) {
            current_state_.styles_applied = true;
            current_state_.last_change = std::chrono::steady_clock::now();
        }
    }
    
    if (event_bus_) {
        event_bus_->emit(EventType::STYLES_APPLIED);
    }
    
    checkOverallReadiness();
}

void BrowserReadinessTracker::updateNetworkIdle() {
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        if (!current_state_.network_idle) {
            current_state_.network_idle = true;
            current_state_.last_change = std::chrono::steady_clock::now();
        }
    }
    
    if (event_bus_) {
        event_bus_->emit(EventType::NETWORK_IDLE);  
    }
    
    checkOverallReadiness();
}

void BrowserReadinessTracker::setupJavaScriptReadinessDetection() {
    javascript_readiness_script_ = generateReadinessCheckScript();
}

std::string BrowserReadinessTracker::generateReadinessCheckScript() const {
    std::ostringstream script;
    
    script << R"JS(
(function() {
    // HeadlessWeb Readiness Detection System
    if (typeof window.hweb_readiness === 'undefined') {
        window.hweb_readiness = {
            dom_ready: false,
            javascript_ready: false,
            resources_loaded: false,
            fonts_loaded: false,
            images_loaded: false,
            styles_applied: false,
            network_idle: false,
            
            // Check functions
            checkAll: function() {
                this.checkDOM();
                this.checkJavaScript(); 
                this.checkResources();
                this.checkFonts();
                this.checkImages();
                this.checkStyles();
                return this.getReadinessState();
            },
            
            checkDOM: function() {
                this.dom_ready = (document.readyState === 'complete' || document.readyState === 'interactive');
                return this.dom_ready;
            },
            
            checkJavaScript: function() {
                try {
                    // Test basic JavaScript functionality
                    var testFunc = function() { return 'ready'; };
                    var result = testFunc();
                    
                    // Test object creation and manipulation
                    var testObj = { test: true };
                    testObj.dynamic = 'value';
                    
                    // Test array operations
                    var testArray = [1, 2, 3];
                    testArray.push(4);
                    
                    // Test DOM manipulation capabilities
                    var canManipulateDOM = typeof document.createElement === 'function' &&
                                          typeof document.querySelector === 'function';
                    
                    this.javascript_ready = (result === 'ready' && 
                                           testObj.dynamic === 'value' && 
                                           testArray.length === 4 &&
                                           canManipulateDOM);
                    
                    return this.javascript_ready;
                } catch(e) {
                    this.javascript_ready = false;
                    return false;
                }
            },
            
            checkResources: function() {
                // Check if all resources have finished loading
                var scripts = document.querySelectorAll('script[src]');
                var stylesheets = document.querySelectorAll('link[rel="stylesheet"]');
                
                var allLoaded = true;
                
                // Check scripts
                for (var i = 0; i < scripts.length; i++) {
                    if (!scripts[i].complete && scripts[i].readyState !== 'complete') {
                        allLoaded = false;
                        break;
                    }
                }
                
                // Check stylesheets
                if (allLoaded) {
                    for (var i = 0; i < stylesheets.length; i++) {
                        var sheet = stylesheets[i];
                        try {
                            // Check if stylesheet is accessible
                            if (sheet.sheet && sheet.sheet.cssRules) {
                                // Stylesheet is loaded
                            } else if (!sheet.sheet) {
                                allLoaded = false;
                                break;
                            }
                        } catch(e) {
                            // Cross-origin stylesheets may throw, but that's ok
                        }
                    }
                }
                
                this.resources_loaded = allLoaded;
                return this.resources_loaded;
            },
            
            checkFonts: function() {
                // Check if fonts are loaded using FontFace API if available
                if (typeof document.fonts !== 'undefined' && document.fonts.ready) {
                    var self = this;
                    document.fonts.ready.then(function() {
                        self.fonts_loaded = true;
                    });
                    this.fonts_loaded = document.fonts.status === 'loaded';
                } else {
                    // Fallback: assume fonts are loaded after DOM is ready
                    this.fonts_loaded = this.dom_ready;
                }
                return this.fonts_loaded;
            },
            
            checkImages: function() {
                var images = document.querySelectorAll('img');
                var allLoaded = true;
                
                for (var i = 0; i < images.length; i++) {
                    var img = images[i];
                    if (!img.complete || img.naturalWidth === 0) {
                        allLoaded = false;
                        break;
                    }
                }
                
                this.images_loaded = allLoaded;
                return this.images_loaded;
            },
            
            checkStyles: function() {
                // Check if styles have been applied by testing computed styles
                try {
                    var body = document.body;
                    if (body) {
                        var computedStyle = window.getComputedStyle(body);
                        // If we can get computed styles, CSS is working
                        this.styles_applied = !!computedStyle;
                    } else {
                        this.styles_applied = false;
                    }
                } catch(e) {
                    this.styles_applied = false;
                }
                return this.styles_applied;
            },
            
            getReadinessState: function() {
                return {
                    dom_ready: this.dom_ready,
                    javascript_ready: this.javascript_ready,
                    resources_loaded: this.resources_loaded,
                    fonts_loaded: this.fonts_loaded,
                    images_loaded: this.images_loaded,
                    styles_applied: this.styles_applied,
                    network_idle: this.network_idle,
                    
                    isInteractive: function() {
                        return this.dom_ready;
                    },
                    
                    isBasicReady: function() {
                        return this.dom_ready && this.javascript_ready;
                    },
                    
                    isFullyReady: function() {
                        return this.dom_ready && this.javascript_ready && 
                               this.resources_loaded && this.fonts_loaded && 
                               this.images_loaded && this.styles_applied && 
                               this.network_idle;
                    }
                };
            }
        };
    }
    
    // Perform readiness check and return state as JSON string
    var state = window.hweb_readiness.checkAll();
    return JSON.stringify(state);
})();
)JS";
    
    return script.str();
}

void BrowserReadinessTracker::checkOverallReadiness() {
    bool was_fully_ready, was_basically_ready, was_interactive;
    
    {
        std::lock_guard<std::mutex> lock(readiness_mutex_);
        was_fully_ready = current_state_.isFullyReady();
        was_basically_ready = current_state_.isBasicReady();
        was_interactive = current_state_.isInteractive();
    }
    
    // Emit overall readiness events
    if (event_bus_) {
        if (was_interactive) {
            event_bus_->emit(EventType::PAGE_INTERACTIVE);
        }
        
        if (was_basically_ready) {
            event_bus_->emit(EventType::PAGE_COMPLETE);
        }
        
        if (was_fully_ready) {
            event_bus_->emit(EventType::BROWSER_READY);
        }
    }
}

void BrowserReadinessTracker::emitReadinessEvents() {
    // This method is called when readiness state changes
    checkOverallReadiness();
}

void BrowserReadinessTracker::setupEventSubscriptions() {
    if (!event_bus_) return;
    
    // Subscribe to relevant events to update readiness state
    event_bus_->subscribe(EventType::DOM_CONTENT_LOADED, 
        [this](const Event& event) {
            updateDOMReady();
        });
    
    event_bus_->subscribe(EventType::NAVIGATION_COMPLETED,
        [this](const Event& event) {
            updateResourcesLoaded();
        });
    
    event_bus_->subscribe(EventType::NETWORK_IDLE,
        [this](const Event& event) {
            updateNetworkIdle();
        });
}

} // namespace BrowserEvents