#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

class Browser; // Forward declaration

class EventLoopManager {
public:
    // Constructor and destructor
    EventLoopManager();
    ~EventLoopManager();
    
    // Non-copyable
    EventLoopManager(const EventLoopManager&) = delete;
    EventLoopManager& operator=(const EventLoopManager&) = delete;
    
    // Initialize with main loop reference
    void initialize(GMainLoop* main_loop);
    
    // Request waiting for JavaScript completion with timeout protection
    bool waitForJavaScriptCompletion(int timeout_ms);
    
    // Request waiting for specific conditions
    bool waitForCondition(std::function<bool()> condition, int timeout_ms = 5000);
    
    // Signal that JavaScript execution is complete
    void signalJavaScriptComplete();
    
    // Check if event loop is currently running
    bool isEventLoopRunning() const;
    
    // Safely quit the event loop if running
    void safeQuit();
    
    // Reset the manager state
    void reset();
    
    // Cleanup resources
    void cleanup();

private:
    GMainLoop* main_loop_ = nullptr;
    std::mutex mutex_;
    std::atomic<bool> is_waiting_{false};
    std::atomic<bool> operation_complete_{false};
    std::atomic<bool> timed_out_{false};
    guint timeout_source_id_ = 0;
    
    // Timeout callback
    static gboolean timeoutCallback(gpointer user_data);
    
    // Internal wait implementation
    bool internalWait(int timeout_ms);
};