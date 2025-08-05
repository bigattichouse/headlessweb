#include "AsyncFileOperations.h"
#include "../Debug.h"
#include "PathUtils.h"
#include <algorithm>
#include <regex>
#include <filesystem>
#include <fstream>

// Platform-specific includes
#ifdef _WIN32
    #include <fileapi.h>
#elif defined(__linux__)
    #include <sys/stat.h>
    #include <unistd.h>
#elif defined(__APPLE__)
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

namespace FileOps {

// ========== FileSystemWatcher Implementation ==========

FileSystemWatcher::FileSystemWatcher(const std::string& directory) 
    : watch_directory_(directory), monitoring_active_(false) {
#ifdef _WIN32
    directory_handle_ = INVALID_HANDLE_VALUE;
    memset(&overlapped_, 0, sizeof(overlapped_));
#elif defined(__linux__)
    inotify_fd_ = -1;
    watch_descriptor_ = -1;
#elif defined(__APPLE__)
    kqueue_fd_ = -1;
    dir_fd_ = -1;
#endif
}

FileSystemWatcher::~FileSystemWatcher() {
    stopWatching();
}

bool FileSystemWatcher::startWatching() {
    if (monitoring_active_.load()) {
        return true; // Already watching
    }
    
    if (!initializePlatformWatcher()) {
        debug_output("Failed to initialize platform file watcher for: " + watch_directory_);
        return false;
    }
    
    monitoring_active_ = true;
    watcher_thread_ = std::thread(&FileSystemWatcher::watcherLoop, this);
    
    debug_output("Started file system watching for: " + watch_directory_);
    return true;
}

void FileSystemWatcher::stopWatching() {
    if (!monitoring_active_.load()) {
        return; // Already stopped
    }
    
    monitoring_active_ = false;
    
    if (watcher_thread_.joinable()) {
        watcher_thread_.join();
    }
    
    cleanupPlatformWatcher();
    debug_output("Stopped file system watching for: " + watch_directory_);
}

void FileSystemWatcher::onFileEvent(std::function<void(const FileEvent&)> callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    event_callbacks_.push_back(callback);
}

void FileSystemWatcher::clearCallbacks() {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    event_callbacks_.clear();
}

std::future<FileEvent> FileSystemWatcher::waitForFileCreated(const std::string& filename_pattern, int timeout_ms) {
    auto promise = std::make_shared<std::promise<FileEvent>>();
    auto future = promise->get_future();
    
    auto callback = [promise, filename_pattern, this](const FileEvent& event) {
        if (event.event_type == FileEventType::CREATED && 
            matchesPattern(event.filepath, filename_pattern)) {
            promise->set_value(event);
        }
    };
    
    onFileEvent(callback);
    
    // Set up timeout
    std::thread([promise, timeout_ms]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        try {
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Timeout waiting for file creation")));
        } catch (...) {
            // Promise may already be set
        }
    }).detach();
    
    return future;
}

std::future<FileEvent> FileSystemWatcher::waitForFileModified(const std::string& filename_pattern, int timeout_ms) {
    auto promise = std::make_shared<std::promise<FileEvent>>();
    auto future = promise->get_future();
    
    auto callback = [promise, filename_pattern, this](const FileEvent& event) {
        if (event.event_type == FileEventType::MODIFIED && 
            matchesPattern(event.filepath, filename_pattern)) {
            promise->set_value(event);
        }
    };
    
    onFileEvent(callback);
    
    // Set up timeout
    std::thread([promise, timeout_ms]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        try {
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Timeout waiting for file modification")));
        } catch (...) {
            // Promise may already be set
        }
    }).detach();
    
    return future;
}

std::future<bool> FileSystemWatcher::waitForFileStable(const std::string& filepath, int stability_ms, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Start async stability monitoring
    std::thread([promise, filepath, stability_ms, timeout_ms, this]() {
        auto start_time = std::chrono::steady_clock::now();
        auto last_modification = std::chrono::steady_clock::now();
        size_t last_size = 0;
        
        try {
            if (std::filesystem::exists(filepath)) {
                last_size = std::filesystem::file_size(filepath);
            }
        } catch (...) {
            promise->set_value(false);
            return;
        }
        
        // Monitor for file changes
        auto callback = [&last_modification, &last_size, filepath](const FileEvent& event) {
            if (event.filepath == filepath && event.event_type == FileEventType::MODIFIED) {
                last_modification = std::chrono::steady_clock::now();
                last_size = event.file_size;
            }
        };
        
        onFileEvent(callback);
        
        while (std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(timeout_ms)) {
            auto now = std::chrono::steady_clock::now();
            auto time_since_modification = now - last_modification;
            
            if (time_since_modification >= std::chrono::milliseconds(stability_ms)) {
                promise->set_value(true);
                return;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        promise->set_value(false); // Timeout
    }).detach();
    
    return future;
}

void FileSystemWatcher::watcherLoop() {
    debug_output("File watcher loop started for: " + watch_directory_);
    
    while (monitoring_active_.load()) {
        try {
            processNativeEvents();
        } catch (const std::exception& e) {
            debug_output("File watcher error: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    debug_output("File watcher loop ended for: " + watch_directory_);
}

void FileSystemWatcher::processNativeEvents() {
#ifdef _WIN32
    // Windows ReadDirectoryChangesW implementation
    DWORD bytes_returned;
    FILE_NOTIFY_INFORMATION* notify_info = (FILE_NOTIFY_INFORMATION*)buffer_;
    
    if (ReadDirectoryChangesW(
        directory_handle_,
        buffer_,
        sizeof(buffer_),
        FALSE, // Don't watch subdirectories
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
        &bytes_returned,
        &overlapped_,
        NULL)) {
        
        while (notify_info != nullptr) {
            std::wstring wide_filename(notify_info->FileName, notify_info->FileNameLength / sizeof(wchar_t));
            std::string filename(wide_filename.begin(), wide_filename.end());
            std::string full_path = watch_directory_ + "/" + filename;
            
            FileEventType event_type = FileEventType::MODIFIED;
            if (notify_info->Action == FILE_ACTION_ADDED) {
                event_type = FileEventType::CREATED;
            } else if (notify_info->Action == FILE_ACTION_REMOVED) {
                event_type = FileEventType::DELETED;
            }
            
            FileEvent event(full_path, event_type);
            emitFileEvent(event);
            
            if (notify_info->NextEntryOffset == 0) break;
            notify_info = (FILE_NOTIFY_INFORMATION*)((char*)notify_info + notify_info->NextEntryOffset);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
#elif defined(__linux__)
    // Linux inotify implementation
    struct pollfd pfd = {inotify_fd_, POLLIN, 0};
    int poll_result = poll(&pfd, 1, 100); // 100ms timeout
    
    if (poll_result > 0) {
        char buffer[1024];
        ssize_t length = read(inotify_fd_, buffer, sizeof(buffer));
        
        if (length > 0) {
            int offset = 0;
            while (offset < length) {
                struct inotify_event* event = (struct inotify_event*)(buffer + offset);
                
                if (event->len > 0) {
                    std::string filename = event->name;
                    std::string full_path = watch_directory_ + "/" + filename;
                    
                    FileEventType event_type = FileEventType::MODIFIED;
                    if (event->mask & IN_CREATE) {
                        event_type = FileEventType::CREATED;
                    } else if (event->mask & IN_DELETE) {
                        event_type = FileEventType::DELETED;
                    } else if (event->mask & IN_MOVED_TO) {
                        event_type = FileEventType::MOVED_TO;
                    }
                    
                    FileEvent file_event(full_path, event_type);
                    // Get file size if file exists
                    try {
                        if (std::filesystem::exists(full_path)) {
                            file_event.file_size = std::filesystem::file_size(full_path);
                        }
                    } catch (...) {
                        file_event.file_size = 0;
                    }
                    
                    emitFileEvent(file_event);
                }
                
                offset += sizeof(struct inotify_event) + event->len;
            }
        }
    }
    
#elif defined(__APPLE__)
    // macOS kqueue implementation
    struct kevent events[10];
    struct timespec timeout = {0, 100000000}; // 100ms
    int event_count = kevent(kqueue_fd_, NULL, 0, events, 10, &timeout);
    
    for (int i = 0; i < event_count; ++i) {
        if (events[i].filter == EVFILT_VNODE) {
            // Directory changed, scan for new files
            FileEvent event(watch_directory_, FileEventType::MODIFIED);
            emitFileEvent(event);
        }
    }
#endif
}

void FileSystemWatcher::emitFileEvent(const FileEvent& event) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    for (const auto& callback : event_callbacks_) {
        try {
            callback(event);
        } catch (const std::exception& e) {
            debug_output("File event callback error: " + std::string(e.what()));
        }
    }
}

bool FileSystemWatcher::initializePlatformWatcher() {
#ifdef _WIN32
    directory_handle_ = CreateFileA(
        watch_directory_.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    return directory_handle_ != INVALID_HANDLE_VALUE;
    
#elif defined(__linux__)
    inotify_fd_ = inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ == -1) {
        return false;
    }
    
    watch_descriptor_ = inotify_add_watch(
        inotify_fd_, 
        watch_directory_.c_str(),
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM
    );
    return watch_descriptor_ != -1;
    
#elif defined(__APPLE__)
    kqueue_fd_ = kqueue();
    if (kqueue_fd_ == -1) {
        return false;
    }
    
    dir_fd_ = open(watch_directory_.c_str(), O_RDONLY);
    if (dir_fd_ == -1) {
        close(kqueue_fd_);
        kqueue_fd_ = -1;
        return false;
    }
    
    struct kevent event;
    EV_SET(&event, dir_fd_, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, NULL);
    return kevent(kqueue_fd_, &event, 1, NULL, 0, NULL) != -1;
    
#else
    return false;
#endif
}

void FileSystemWatcher::cleanupPlatformWatcher() {
#ifdef _WIN32
    if (directory_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(directory_handle_);
        directory_handle_ = INVALID_HANDLE_VALUE;
    }
    
#elif defined(__linux__)
    if (watch_descriptor_ != -1) {
        inotify_rm_watch(inotify_fd_, watch_descriptor_);
        watch_descriptor_ = -1;
    }
    if (inotify_fd_ != -1) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
    
#elif defined(__APPLE__)
    if (dir_fd_ != -1) {
        close(dir_fd_);
        dir_fd_ = -1;
    }
    if (kqueue_fd_ != -1) {
        close(kqueue_fd_);
        kqueue_fd_ = -1;
    }
#endif
}

bool FileSystemWatcher::matchesPattern(const std::string& filename, const std::string& pattern) const {
    // Simple pattern matching - convert glob patterns to regex
    if (pattern.empty()) return true;
    if (pattern == "*") return true;
    
    std::string regex_pattern = pattern;
    // Escape special regex characters except * and ?
    std::regex special_chars{R"([-[\]{}()+.,\^$|#\s])"};
    regex_pattern = std::regex_replace(regex_pattern, special_chars, R"(\$&)");
    
    // Convert glob wildcards to regex
    std::regex_replace(regex_pattern, std::regex(R"(\*)"), ".*");
    std::regex_replace(regex_pattern, std::regex(R"(\?)"), ".");
    
    try {
        std::regex pattern_regex(regex_pattern, std::regex_constants::icase);
        return std::regex_search(filename, pattern_regex);
    } catch (...) {
        // Fallback to simple string matching
        return filename.find(pattern) != std::string::npos;
    }
}

// ========== DownloadCompletionDetector Implementation ==========

DownloadCompletionDetector::DownloadCompletionDetector(const std::string& download_directory)
    : detection_active_(false), stability_duration_(std::chrono::milliseconds(1000)) {
    
    watcher_ = std::make_shared<FileSystemWatcher>(download_directory);
    
    // Set common browser temp file patterns
    temp_file_patterns_ = {
        ".crdownload",  // Chrome
        ".part",        // Firefox
        ".download",    // Safari
        ".partial",     // Edge
        ".tmp",         // Generic
        ".temp"         // Generic
    };
}

DownloadCompletionDetector::~DownloadCompletionDetector() {
    detection_active_ = false;
    if (watcher_) {
        watcher_->stopWatching();
    }
}

std::future<std::string> DownloadCompletionDetector::waitForDownload(const std::string& filename_pattern, int timeout_ms) {
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    
    if (!watcher_->startWatching()) {
        promise->set_exception(std::make_exception_ptr(std::runtime_error("Failed to start file watcher")));
        return future;
    }
    
    auto callback = [promise, filename_pattern, this](const FileEvent& event) {
        if (event.event_type == FileEventType::CREATED || event.event_type == FileEventType::MODIFIED) {
            if (watcher_->matchesPattern(event.filepath, filename_pattern) && 
                !isBrowserTempFile(event.filepath)) {
                promise->set_value(event.filepath);
            }
        }
    };
    
    watcher_->onFileEvent(callback);
    
    // Set up timeout
    std::thread([promise, timeout_ms]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        try {
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Timeout waiting for download")));
        } catch (...) {
            // Promise may already be set
        }
    }).detach();
    
    return future;
}

std::future<bool> DownloadCompletionDetector::waitForDownloadComplete(const std::string& filepath, int timeout_ms) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    
    // Start async completion monitoring
    std::thread([promise, filepath, timeout_ms, this]() {
        auto start_time = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(timeout_ms)) {
            try {
                // Check if file exists and is readable
                if (!std::filesystem::exists(filepath)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                
                // Check if it's still a temp file
                if (isBrowserTempFile(filepath)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                
                // Check if file size is stable
                if (isFileSizeStable(filepath, stability_duration_)) {
                    // Ensure file is not empty
                    if (getFileSize(filepath) > 0) {
                        promise->set_value(true);
                        return;
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        promise->set_value(false); // Timeout
    }).detach();
    
    return future;
}

bool DownloadCompletionDetector::isBrowserTempFile(const std::string& filepath) const {
    std::string filename = std::filesystem::path(filepath).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    for (const auto& pattern : temp_file_patterns_) {
        if (filename.size() >= pattern.size() && 
            filename.substr(filename.size() - pattern.size()) == pattern) {
            return true;
        }
    }
    
    return false;
}

bool DownloadCompletionDetector::isFileSizeStable(const std::string& filepath, std::chrono::milliseconds duration) const {
    try {
        size_t initial_size = getFileSize(filepath);
        auto start_time = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - start_time < duration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            size_t current_size = getFileSize(filepath);
            
            if (current_size != initial_size) {
                // Size changed, restart stability check
                initial_size = current_size;
                start_time = std::chrono::steady_clock::now();
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

size_t DownloadCompletionDetector::getFileSize(const std::string& filepath) const {
    try {
        return std::filesystem::file_size(filepath);
    } catch (...) {
        return 0;
    }
}

// ========== AsyncFileOperations Implementation ==========

AsyncFileOperations::AsyncFileOperations(const std::string& download_directory) 
    : default_download_directory_(download_directory) {
    
    if (default_download_directory_.empty()) {
        // Try to detect default download directory
        default_download_directory_ = std::getenv("HOME") ? 
            std::string(std::getenv("HOME")) + "/Downloads" : "/tmp";
    }
    
    initializeDownloadDetector();
}

AsyncFileOperations::~AsyncFileOperations() {
    if (watcher_) {
        watcher_->stopWatching();
    }
}

std::future<std::string> AsyncFileOperations::waitForDownload(const std::string& filename_pattern, int timeout_ms) {
    if (!download_detector_) {
        auto promise = std::make_shared<std::promise<std::string>>();
        promise->set_exception(std::make_exception_ptr(std::runtime_error("Download detector not initialized")));
        return promise->get_future();
    }
    
    return download_detector_->waitForDownload(filename_pattern, timeout_ms);
}

std::future<bool> AsyncFileOperations::waitForDownloadComplete(const std::string& filepath, int timeout_ms) {
    if (!download_detector_) {
        auto promise = std::make_shared<std::promise<bool>>();
        promise->set_exception(std::make_exception_ptr(std::runtime_error("Download detector not initialized")));
        return promise->get_future();
    }
    
    return download_detector_->waitForDownloadComplete(filepath, timeout_ms);
}

void AsyncFileOperations::initializeDownloadDetector() {
    try {
        download_detector_ = std::make_unique<DownloadCompletionDetector>(default_download_directory_);
        debug_output("Initialized async file operations for: " + default_download_directory_);
    } catch (const std::exception& e) {
        debug_output("Failed to initialize download detector: " + std::string(e.what()));
    }
}

std::string AsyncFileOperations::resolveDownloadDirectory(const std::string& directory) const {
    return directory.empty() ? default_download_directory_ : directory;
}

bool AsyncFileOperations::fileMatchesPattern(const std::string& filepath, const std::string& pattern) {
    FileSystemWatcher dummy_watcher("");
    return dummy_watcher.matchesPattern(filepath, pattern);
}

} // namespace FileOps