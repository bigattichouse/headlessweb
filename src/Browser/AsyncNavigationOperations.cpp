#include "BrowserEventBus.h"
#include <algorithm>
#include <thread>
#include <sstream>

namespace BrowserEvents {

// ========== AsyncNavigationOperations Implementation ==========

std::future<bool> AsyncNavigationOperations::waitForPageLoadComplete(const std::string& url, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for page load completion
    event_bus_->subscribeOnce(EventType::PAGE_LOAD_COMPLETE,
        [promise, url](const Event& event) {
            if (auto page_event = dynamic_cast<const PageLoadEvent*>(&event)) {
                if (url.empty() || page_event->url == url) {
                    try {
                        promise->set_value(true);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [url](const Event& event) {
            return url.empty() || event.target == url;
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

std::future<bool> AsyncNavigationOperations::waitForViewportReady(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for viewport ready
    event_bus_->subscribeOnce(EventType::VIEWPORT_READY,
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

std::future<bool> AsyncNavigationOperations::waitForRenderingComplete(int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for rendering completion
    event_bus_->subscribeOnce(EventType::RENDERING_COMPLETE,
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

std::future<bool> AsyncNavigationOperations::waitForSPANavigation(const std::string& route, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for SPA route change
    event_bus_->subscribeOnce(EventType::SPA_ROUTE_CHANGED,
        [promise, route](const Event& event) {
            if (auto page_event = dynamic_cast<const PageLoadEvent*>(&event)) {
                if (route.empty() || page_event->url.find(route) != std::string::npos) {
                    try {
                        promise->set_value(true);
                    } catch (const std::future_error&) {
                        // Promise already set
                    }
                }
            }
        },
        [route](const Event& event) {
            return route.empty() || event.target.find(route) != std::string::npos;
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

std::future<bool> AsyncNavigationOperations::waitForFrameworkReady(const std::string& framework, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    if (!event_bus_) {
        promise->set_value(false);
        return std::move(future);
    }
    
    // Set up event listener for framework detection
    event_bus_->subscribeOnce(EventType::FRAMEWORK_DETECTED,
        [promise, framework](const Event& event) {
            if (framework.empty() || event.data.find(framework) != std::string::npos) {
                try {
                    promise->set_value(true);
                } catch (const std::future_error&) {
                    // Promise already set
                }
            }
        },
        [framework](const Event& event) {
            return framework.empty() || event.data.find(framework) != std::string::npos;
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

std::future<PageLoadEvent> AsyncNavigationOperations::waitForPageLoadEvent(EventType event_type, int timeout_ms) {
    auto promise = std::make_shared<EventPromise<PageLoadEvent>>();
    
    if (event_bus_) {
        event_bus_->subscribeOnce(event_type,
            [promise, event_type](const Event& event) {
                if (auto page_event = dynamic_cast<const PageLoadEvent*>(&event)) {
                    promise->resolve(*page_event);
                } else {
                    // Convert generic event to PageLoadEvent
                    PageLoadEvent converted_event(event_type, event.target, 1.0, "complete", false);
                    promise->resolve(converted_event);
                }
            });
    }
    
    // Set up timeout
    if (timeout_ms > 0) {
        std::thread([promise, timeout_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            promise->reject("Page load event timeout after " + std::to_string(timeout_ms) + "ms");
        }).detach();
    }
    
    return std::move(promise->getFuture());
}

// Page load state monitoring methods
void AsyncNavigationOperations::emitPageLoadStarted(const std::string& url) {
    emitPageLoadEvent(EventType::PAGE_LOAD_STARTED, url, 0.0, "started");
}

void AsyncNavigationOperations::emitPageLoadProgress(const std::string& url, double progress) {
    emitPageLoadEvent(EventType::PAGE_LOAD_PROGRESS, url, progress, "progress");
}

void AsyncNavigationOperations::emitPageLoadComplete(const std::string& url) {
    emitPageLoadEvent(EventType::PAGE_LOAD_COMPLETE, url, 1.0, "complete");
}

void AsyncNavigationOperations::emitViewportReady() {
    if (event_bus_) {
        event_bus_->emit(EventType::VIEWPORT_READY);
    }
}

void AsyncNavigationOperations::emitRenderingComplete() {
    if (event_bus_) {
        event_bus_->emit(EventType::RENDERING_COMPLETE);
    }
}

void AsyncNavigationOperations::emitSPARouteChanged(const std::string& old_route, const std::string& new_route) {
    emitPageLoadEvent(EventType::SPA_ROUTE_CHANGED, new_route, 1.0, "spa_navigation", true);
}

// JavaScript generation methods
std::string AsyncNavigationOperations::generatePageLoadMonitorScript() const {
    return R"JS(
(function() {
    // HeadlessWeb Page Load Monitoring
    if (typeof window.hweb_navigation_monitor === 'undefined') {
        window.hweb_navigation_monitor = {
            start_time: Date.now(),
            resources_loaded: 0,
            total_resources: 0,
            load_complete: false,
            
            init: function() {
                // Monitor resource loading
                var resources = document.querySelectorAll('img, script[src], link[rel="stylesheet"]');
                this.total_resources = resources.length;
                
                // Track resource loading
                for (var i = 0; i < resources.length; i++) {
                    var resource = resources[i];
                    if (resource.complete || resource.readyState === 'complete') {
                        this.resources_loaded++;
                    } else {
                        resource.addEventListener('load', this.onResourceLoad.bind(this));
                        resource.addEventListener('error', this.onResourceLoad.bind(this));
                    }
                }
                
                // Monitor DOM content loaded
                if (document.readyState === 'loading') {
                    document.addEventListener('DOMContentLoaded', this.onDOMReady.bind(this));
                } else {
                    this.onDOMReady();
                }
                
                // Monitor window load
                if (document.readyState === 'complete') {
                    this.onWindowLoad();
                } else {
                    window.addEventListener('load', this.onWindowLoad.bind(this));
                }
            },
            
            onResourceLoad: function() {
                this.resources_loaded++;
                this.checkLoadComplete();
            },
            
            onDOMReady: function() {
                if (typeof window.hweb_emit_page_event === 'function') {
                    window.hweb_emit_page_event('DOM_READY', window.location.href, 0.5);
                }
            },
            
            onWindowLoad: function() {
                this.load_complete = true;
                this.checkLoadComplete();
            },
            
            checkLoadComplete: function() {
                var progress = this.total_resources > 0 ? this.resources_loaded / this.total_resources : 1.0;
                
                if (typeof window.hweb_emit_page_event === 'function') {
                    window.hweb_emit_page_event('PAGE_LOAD_PROGRESS', window.location.href, progress);
                }
                
                if (this.load_complete && this.resources_loaded >= this.total_resources) {
                    if (typeof window.hweb_emit_page_event === 'function') {
                        window.hweb_emit_page_event('PAGE_LOAD_COMPLETE', window.location.href, 1.0);
                    }
                }
            },
            
            getProgress: function() {
                return {
                    progress: this.total_resources > 0 ? this.resources_loaded / this.total_resources : 1.0,
                    resources_loaded: this.resources_loaded,
                    total_resources: this.total_resources,
                    load_complete: this.load_complete,
                    elapsed_time: Date.now() - this.start_time
                };
            }
        };
        
        // Initialize monitoring
        window.hweb_navigation_monitor.init();
    }
    
    return window.hweb_navigation_monitor.getProgress();
})();
)JS";
}

std::string AsyncNavigationOperations::generateSPANavigationDetectionScript() const {
    return R"JS(
(function() {
    // HeadlessWeb SPA Navigation Detection
    if (typeof window.hweb_spa_monitor === 'undefined') {
        window.hweb_spa_monitor = {
            current_url: window.location.href,
            current_hash: window.location.hash,
            
            init: function() {
                // Monitor pushState/replaceState for SPA navigation
                var originalPushState = history.pushState;
                var originalReplaceState = history.replaceState;
                var self = this;
                
                history.pushState = function() {
                    originalPushState.apply(history, arguments);
                    self.onURLChange();
                };
                
                history.replaceState = function() {
                    originalReplaceState.apply(history, arguments);
                    self.onURLChange();
                };
                
                // Monitor popstate (back/forward buttons)
                window.addEventListener('popstate', function() {
                    self.onURLChange();
                });
                
                // Monitor hash changes
                window.addEventListener('hashchange', function() {
                    self.onHashChange();
                });
            },
            
            onURLChange: function() {
                var new_url = window.location.href;
                if (new_url !== this.current_url) {
                    var old_url = this.current_url;
                    this.current_url = new_url;
                    
                    if (typeof window.hweb_emit_page_event === 'function') {
                        window.hweb_emit_page_event('SPA_ROUTE_CHANGED', new_url, 1.0, old_url);
                    }
                }
            },
            
            onHashChange: function() {
                var new_hash = window.location.hash;
                if (new_hash !== this.current_hash) {
                    var old_hash = this.current_hash;
                    this.current_hash = new_hash;
                    
                    if (typeof window.hweb_emit_page_event === 'function') {
                        window.hweb_emit_page_event('SPA_ROUTE_CHANGED', window.location.href, 1.0, old_hash);
                    }
                }
            }
        };
        
        // Initialize SPA monitoring
        window.hweb_spa_monitor.init();
    }
    
    return {
        current_url: window.hweb_spa_monitor.current_url,
        current_hash: window.hweb_spa_monitor.current_hash
    };
})();
)JS";
}

std::string AsyncNavigationOperations::generateFrameworkDetectionScript(const std::string& framework) const {
    std::ostringstream script;
    
    script << R"JS(
(function(targetFramework) {
    // HeadlessWeb Framework Detection
    var frameworks = {
        react: function() {
            return typeof window.React !== 'undefined' || 
                   document.querySelector('[data-reactroot]') !== null ||
                   document.querySelector('._reactContainer') !== null;
        },
        
        vue: function() {
            return typeof window.Vue !== 'undefined' ||
                   document.querySelector('[data-v-]') !== null ||
                   document.querySelector('.__vue__') !== null;
        },
        
        angular: function() {
            return typeof window.angular !== 'undefined' ||
                   typeof window.ng !== 'undefined' ||
                   document.querySelector('[ng-app]') !== null ||
                   document.querySelector('app-root') !== null;
        },
        
        jquery: function() {
            return typeof window.jQuery !== 'undefined' || typeof window.$ !== 'undefined';
        },
        
        backbone: function() {
            return typeof window.Backbone !== 'undefined';
        },
        
        ember: function() {
            return typeof window.Ember !== 'undefined';
        }
    };
    
    // Check specific framework or all frameworks
    if (targetFramework && frameworks[targetFramework.toLowerCase()]) {
        var detected = frameworks[targetFramework.toLowerCase()]();
        if (detected && typeof window.hweb_emit_page_event === 'function') {
            window.hweb_emit_page_event('FRAMEWORK_DETECTED', window.location.href, 1.0, targetFramework);
        }
        return detected;
    } else {
        // Check all frameworks
        var detected_frameworks = [];
        for (var name in frameworks) {
            if (frameworks[name]()) {
                detected_frameworks.push(name);
            }
        }
        
        if (detected_frameworks.length > 0 && typeof window.hweb_emit_page_event === 'function') {
            window.hweb_emit_page_event('FRAMEWORK_DETECTED', window.location.href, 1.0, detected_frameworks.join(','));
        }
        
        return detected_frameworks;
    }
})()JS" << "('" << framework << "');";
    
    return script.str();
}

std::string AsyncNavigationOperations::generateRenderingCompleteScript() const {
    return R"JS(
(function() {
    // HeadlessWeb Rendering Completion Detection
    if (typeof window.hweb_rendering_monitor === 'undefined') {
        window.hweb_rendering_monitor = {
            last_dom_change: Date.now(),
            observer: null,
            check_interval: null,
            stability_threshold: 500, // ms
            
            init: function() {
                var self = this;
                
                // Set up MutationObserver to detect DOM changes
                this.observer = new MutationObserver(function(mutations) {
                    self.last_dom_change = Date.now();
                });
                
                this.observer.observe(document.body || document.documentElement, {
                    childList: true,
                    subtree: true,
                    attributes: true,
                    characterData: true
                });
                
                // Check for stability periodically
                this.check_interval = setInterval(function() {
                    self.checkStability();
                }, 100);
            },
            
            checkStability: function() {
                var now = Date.now();
                var time_since_change = now - this.last_dom_change;
                
                // If DOM has been stable for threshold time, consider rendering complete
                if (time_since_change >= this.stability_threshold) {
                    if (typeof window.hweb_emit_page_event === 'function') {
                        window.hweb_emit_page_event('RENDERING_COMPLETE', window.location.href, 1.0);
                    }
                    
                    // Clean up
                    if (this.observer) {
                        this.observer.disconnect();
                        this.observer = null;
                    }
                    if (this.check_interval) {
                        clearInterval(this.check_interval);
                        this.check_interval = null;
                    }
                }
            },
            
            getStatus: function() {
                return {
                    time_since_change: Date.now() - this.last_dom_change,
                    stability_threshold: this.stability_threshold,
                    is_stable: (Date.now() - this.last_dom_change) >= this.stability_threshold
                };
            }
        };
        
        // Initialize rendering monitoring
        window.hweb_rendering_monitor.init();
    }
    
    return window.hweb_rendering_monitor.getStatus();
})();
)JS";
}

// Private method
void AsyncNavigationOperations::emitPageLoadEvent(EventType type, const std::string& url, double progress, 
                                                  const std::string& state, bool spa) {
    if (event_bus_) {
        PageLoadEvent event(type, url, progress, state, spa);
        event_bus_->emit(event);
    }
}

} // namespace BrowserEvents