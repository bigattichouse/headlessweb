#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <functional>
#include <future>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <sys/inotify.h>
    #include <poll.h>
#elif defined(__APPLE__)
    #include <sys/event.h>
#endif

namespace FileOps {

// Event-driven file operation result types
enum class FileOperationResult {
    SUCCESS,
    TIMEOUT,
    FILE_NOT_FOUND,
    DIRECTORY_NOT_FOUND,
    PERMISSION_DENIED,
    WATCHER_FAILED,
    UNKNOWN_ERROR
};

// File system event types
enum class FileEventType {
    CREATED,
    MODIFIED,
    DELETED,
    MOVED_FROM,
    MOVED_TO,
    ATTRIBUTES_CHANGED
};

// File event structure
struct FileEvent {
    std::string filepath;
    FileEventType event_type;
    std::chrono::steady_clock::time_point timestamp;
    size_t file_size;
    
    FileEvent(const std::string& path, FileEventType type) 
        : filepath(path), event_type(type), timestamp(std::chrono::steady_clock::now()), file_size(0) {}
};

// DownloadProgress struct is defined in Types.h

// Event-driven file system watcher
class FileSystemWatcher {
private:
    std::string watch_directory_;
    std::atomic<bool> monitoring_active_;
    std::thread watcher_thread_;
    mutable std::mutex callbacks_mutex_;
    
    // Platform-specific handles
#ifdef _WIN32
    HANDLE directory_handle_;
    OVERLAPPED overlapped_;
    char buffer_[1024];
#elif defined(__linux__)
    int inotify_fd_;
    int watch_descriptor_;
#elif defined(__APPLE__)
    int kqueue_fd_;
    int dir_fd_;
#endif
    
    // Event callbacks
    std::vector<std::function<void(const FileEvent&)>> event_callbacks_;
    
public:
    FileSystemWatcher(const std::string& directory);
    ~FileSystemWatcher();
    
    // Watcher control
    bool startWatching();
    void stopWatching();
    bool isWatching() const { return monitoring_active_.load(); }
    
    // Event subscription
    void onFileEvent(std::function<void(const FileEvent&)> callback);
    void clearCallbacks();
    
    // Convenience methods
    std::future<FileEvent> waitForFileCreated(const std::string& filename_pattern, int timeout_ms = 10000);
    std::future<FileEvent> waitForFileModified(const std::string& filename_pattern, int timeout_ms = 10000);
    std::future<bool> waitForFileStable(const std::string& filepath, int stability_ms = 1000, int timeout_ms = 30000);
    
    // Pattern matching utility
    bool matchesPattern(const std::string& filename, const std::string& pattern) const;
    
private:
    void watcherLoop();
    void processNativeEvents();
    void emitFileEvent(const FileEvent& event);
    bool initializePlatformWatcher();
    void cleanupPlatformWatcher();
};

// Event-driven download completion detector
class DownloadCompletionDetector {
private:
    std::shared_ptr<FileSystemWatcher> watcher_;
    std::atomic<bool> detection_active_;
    
public:
    explicit DownloadCompletionDetector(const std::string& download_directory);
    ~DownloadCompletionDetector();
    
    // Download detection methods
    std::future<std::string> waitForDownload(const std::string& filename_pattern, int timeout_ms = 30000);
    std::future<std::vector<std::string>> waitForMultipleDownloads(
        const std::vector<std::string>& patterns, int timeout_ms = 60000);
    
    // Completion detection
    std::future<bool> waitForDownloadComplete(const std::string& filepath, int timeout_ms = 30000);
    std::future<DownloadProgress> monitorDownloadProgress(
        const std::string& filepath, std::function<void(const DownloadProgress&)> progress_callback = nullptr);
    
    // Configuration
    void setStabilityCheckDuration(std::chrono::milliseconds duration);
    void setBrowserTempFilePatterns(const std::vector<std::string>& patterns);
    
private:
    std::chrono::milliseconds stability_duration_;
    std::vector<std::string> temp_file_patterns_;
    
    bool isBrowserTempFile(const std::string& filepath) const;
    bool isFileSizeStable(const std::string& filepath, std::chrono::milliseconds duration) const;
    size_t getFileSize(const std::string& filepath) const;
};

// Main event-driven file operations class
class AsyncFileOperations {
private:
    std::string default_download_directory_;
    std::shared_ptr<FileSystemWatcher> watcher_;
    std::unique_ptr<DownloadCompletionDetector> download_detector_;
    
public:
    explicit AsyncFileOperations(const std::string& download_directory = "");
    ~AsyncFileOperations();
    
    // File existence and monitoring
    std::future<bool> waitForFileExists(const std::string& filepath, int timeout_ms = 10000);
    std::future<bool> waitForFileDeleted(const std::string& filepath, int timeout_ms = 10000);
    std::future<FileEvent> waitForFileEvent(const std::string& filepath, FileEventType event_type, int timeout_ms = 10000);
    
    // Download operations (replaces DownloadManager polling)
    std::future<std::string> waitForDownload(const std::string& filename_pattern, int timeout_ms = 30000);
    std::future<std::vector<std::string>> waitForMultipleDownloads(
        const std::vector<std::string>& patterns, int timeout_ms = 60000);
    std::future<bool> waitForDownloadComplete(const std::string& filepath, int timeout_ms = 30000);
    
    // Progress monitoring
    void monitorDownloadProgress(const std::string& filepath, 
                               std::function<void(const DownloadProgress&)> progress_callback);
    
    // Configuration
    void setDefaultDownloadDirectory(const std::string& directory);
    std::string getDefaultDownloadDirectory() const { return default_download_directory_; }
    
    // Utility methods
    static bool fileMatchesPattern(const std::string& filepath, const std::string& pattern);
    static std::vector<std::string> findMatchingFiles(const std::string& directory, const std::string& pattern);
    static FileOperationResult translateErrorCode(int error_code);
    
private:
    void initializeDownloadDetector();
    std::string resolveDownloadDirectory(const std::string& directory = "") const;
};

} // namespace FileOps