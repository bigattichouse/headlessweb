#include "EventLoopManager.h"
#include <iostream>
#include <chrono>
#include <thread>

// External debug flag
extern bool g_debug;

void debug_output(const std::string& message);

EventLoopManager::EventLoopManager() {
    debug_output("EventLoopManager instance created");
}

EventLoopManager::~EventLoopManager() {
    cleanup();
    debug_output("EventLoopManager instance destroyed");
}

void EventLoopManager::initialize(GMainLoop* main_loop) {
    std::lock_guard<std::mutex> lock(mutex_);
    main_loop_ = main_loop;
    debug_output("EventLoopManager initialized");
}

bool EventLoopManager::waitForJavaScriptCompletion(int timeout_ms) {
    // Check if already waiting first (without lock to avoid deadlock)
    if (is_waiting_.load()) {
        debug_output("EventLoopManager: Already waiting, skipping nested wait");
        return true; // Assume success to prevent hangs
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!main_loop_) {
        debug_output("EventLoopManager: Main loop not initialized");
        return false;
    }
    
    return internalWait(timeout_ms);
}

bool EventLoopManager::waitForCondition(std::function<bool()> condition, int timeout_ms) {
    if (!condition) {
        return false;
    }
    
    // Check condition immediately
    if (condition()) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!main_loop_) {
        debug_output("EventLoopManager: Main loop not initialized");
        return false;
    }
    
    // If already waiting, don't nest - use polling instead
    if (is_waiting_.load()) {
        debug_output("EventLoopManager: Already waiting, using polling for condition");
        
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start_time).count() < timeout_ms) {
            
            if (condition()) {
                return true;
            }
            
            // Process pending events without blocking
            while (g_main_context_pending(g_main_context_default())) {
                g_main_context_iteration(g_main_context_default(), FALSE);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return false;
    }
    
    // Set up condition checking with timeout
    struct ConditionData {
        std::function<bool()> condition;
        EventLoopManager* manager;
        bool result;
    };
    
    ConditionData* data = new ConditionData{condition, this, false};
    
    // Set up periodic condition check
    guint check_source_id = g_timeout_add(50, [](gpointer user_data) -> gboolean {
        ConditionData* data = static_cast<ConditionData*>(user_data);
        
        if (data->condition()) {
            data->result = true;
            data->manager->signalJavaScriptComplete();
            return G_SOURCE_REMOVE;
        }
        
        return G_SOURCE_CONTINUE; // Keep checking
    }, data);
    
    bool success = internalWait(timeout_ms);
    bool result = data->result;
    
    // Cleanup
    delete data;
    
    return success && result;
}

void EventLoopManager::signalJavaScriptComplete() {
    operation_complete_.store(true);
    
    if (is_waiting_.load() && main_loop_ && g_main_loop_is_running(main_loop_)) {
        g_main_loop_quit(main_loop_);
    }
}

bool EventLoopManager::isEventLoopRunning() const {
    return main_loop_ && g_main_loop_is_running(main_loop_);
}

void EventLoopManager::safeQuit() {
    if (main_loop_ && g_main_loop_is_running(main_loop_)) {
        g_main_loop_quit(main_loop_);
    }
}

void EventLoopManager::reset() {
    is_waiting_.store(false);
    operation_complete_.store(false);
    timed_out_.store(false);
    timeout_source_id_ = 0;
}

void EventLoopManager::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Explicitly remove timeout source before unreffing main_loop
    if (timeout_source_id_ != 0) {
        g_source_remove(timeout_source_id_);
        timeout_source_id_ = 0;
    }

    if (main_loop_) {
        g_main_loop_unref(main_loop_);
        main_loop_ = nullptr;
    }
    reset(); // Reset other state variables
}

gboolean EventLoopManager::timeoutCallback(gpointer user_data) {
    EventLoopManager* manager = static_cast<EventLoopManager*>(user_data);
    
    manager->timed_out_.store(true);
    manager->timeout_source_id_ = 0;
    
    if (manager->main_loop_ && g_main_loop_is_running(manager->main_loop_)) {
        g_main_loop_quit(manager->main_loop_);
    }
    
    return G_SOURCE_REMOVE;
}

bool EventLoopManager::internalWait(int timeout_ms) {
    // Reset state
    operation_complete_.store(false);
    timed_out_.store(false);
    is_waiting_.store(true);
    
    // Set up timeout
    timeout_source_id_ = g_timeout_add(timeout_ms, timeoutCallback, this);
    
    // EVENT-DRIVEN APPROACH: Process events non-blocking with timeout
    auto start_time = std::chrono::steady_clock::now();
    while (!timed_out_.load() && !operation_complete_.load() &&
           std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < timeout_ms) {
        // Process pending events non-blocking
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        
        // Small sleep to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Clean up timeout if still active
    if (timeout_source_id_ != 0) {
        if (g_source_remove(timeout_source_id_)) {
        }
        timeout_source_id_ = 0;
    }
    
    is_waiting_.store(false);
    
    bool success = operation_complete_.load() && !timed_out_.load();
    
    return success;
}