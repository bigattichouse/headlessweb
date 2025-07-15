#include "DownloadManager.h"
#include "../Debug.h"
#include <algorithm>
#include <regex>
#include <fstream>
#include <iostream>
#include <future>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <fileapi.h>
#elif defined(__linux__)
    #include <sys/inotify.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <poll.h>
#elif defined(__APPLE__)
    #include <sys/event.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

namespace FileOps {
    
    // ========== Main Download Interface ==========
    
    DownloadResult DownloadManager::waitForDownload(const DownloadCommand& cmd) {
        debug_output("Starting download monitoring for pattern: " + cmd.filename_pattern);
        
        // Determine download directory
        std::string download_dir = cmd.download_dir;
        if (download_dir.empty()) {
            download_dir = getDownloadDirectory();
        }
        
        if (!PathUtils::isDirectory(download_dir)) {
            debug_output("Download directory not found: " + download_dir);
            return DownloadResult::DIRECTORY_NOT_FOUND;
        }
        
        // Check if file already exists
        auto existing_files = findMatchingFiles(download_dir, cmd.filename_pattern);
        if (!existing_files.empty()) {
            std::string filepath = existing_files[0];
            debug_output("File already exists: " + filepath);
            
            if (verifyDownloadIntegrity(filepath, cmd.expected_size)) {
                return DownloadResult::SUCCESS;
            }
        }
        
        // Start monitoring for new files
        std::string found_file;
        bool file_found = false;
        
        auto file_found_callback = [&](const std::string& filepath) {
            if (!file_found && fileMatchesPattern(filepath, cmd.filename_pattern)) {
                found_file = filepath;
                file_found = true;
                debug_output("Download detected: " + filepath);
            }
        };
        
        // Try native file watching first
        bool using_native_watching = false;
        if (isNativeFileWatchingAvailable()) {
            using_native_watching = monitorDirectoryForNewFiles(
                download_dir, cmd.filename_pattern, cmd.timeout_ms, file_found_callback
            );
        }
        
        // Fallback to polling if native watching failed
        if (!using_native_watching) {
            debug_output("Using polling fallback for file monitoring");
            if (!startPollingFileMonitor(
                download_dir, cmd.filename_pattern, polling_interval_ms_, 
                cmd.timeout_ms, file_found_callback)) {
                return DownloadResult::TIMEOUT;
            }
        }
        
        // If file was found, wait for completion
        if (file_found && !found_file.empty()) {
            if (waitForDownloadCompletion(found_file, cmd.timeout_ms)) {
                if (cmd.verify_integrity && !verifyDownloadIntegrity(found_file, cmd.expected_size)) {
                    return DownloadResult::INTEGRITY_CHECK_FAILED;
                }
                
                // Call completion hook if set
                if (completion_hook_) {
                    completion_hook_(found_file);
                }
                
                updateDownloadStats(true, false);
                return DownloadResult::SUCCESS;
            } else {
                updateDownloadStats(false, true);
                return DownloadResult::TIMEOUT;
            }
        }
        
        updateDownloadStats(false, true);
        return DownloadResult::FILE_NOT_FOUND;
    }
    
    DownloadResult DownloadManager::waitForMultipleDownloads(
        const std::vector<std::string>& patterns,
        const std::string& download_dir,
        int timeout_ms) {
        
        if (patterns.empty()) {
            return DownloadResult::SUCCESS;
        }
        
        std::string dir = download_dir.empty() ? getDownloadDirectory() : download_dir;
        std::vector<bool> pattern_found(patterns.size(), false);
        std::vector<std::string> found_files(patterns.size());
        
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms);
        
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            for (size_t i = 0; i < patterns.size(); ++i) {
                if (!pattern_found[i]) {
                    auto matching_files = findMatchingFiles(dir, patterns[i]);
                    if (!matching_files.empty()) {
                        found_files[i] = matching_files[0];
                        pattern_found[i] = true;
                        debug_output("Found download " + std::to_string(i + 1) + ": " + found_files[i]);
                    }
                }
            }
            
            // Check if all patterns found
            bool all_found = std::all_of(pattern_found.begin(), pattern_found.end(), 
                                        [](bool found) { return found; });
            if (all_found) {
                // Wait for all files to complete
                bool all_complete = true;
                for (const auto& filepath : found_files) {
                    if (!filepath.empty() && !waitForDownloadCompletion(filepath, 5000)) {
                        all_complete = false;
                    }
                }
                
                return all_complete ? DownloadResult::SUCCESS : DownloadResult::TIMEOUT;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(polling_interval_ms_));
        }
        
        return DownloadResult::TIMEOUT;
    }
    
    void DownloadManager::startAsyncDownloadMonitoring(
        const DownloadCommand& cmd,
        std::function<void(DownloadResult, const std::string&)> callback) {
        
        monitoring_active_ = true;
        
        auto monitoring_thread = std::thread([this, cmd, callback]() {
            DownloadResult result = waitForDownload(cmd);
            std::string found_file;
            
            if (result == DownloadResult::SUCCESS) {
                auto files = findMatchingFiles(
                    cmd.download_dir.empty() ? getDownloadDirectory() : cmd.download_dir,
                    cmd.filename_pattern
                );
                if (!files.empty()) {
                    found_file = files[0];
                }
            }
            
            callback(result, found_file);
        });
        
        monitoring_threads_.push_back(std::move(monitoring_thread));
    }
    
    void DownloadManager::cancelDownloadMonitoring() {
        monitoring_active_ = false;
        
        // Wait for all monitoring threads to complete
        for (auto& thread : monitoring_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        monitoring_threads_.clear();
        
        // Clean up platform-specific resources
        cleanupPlatformWatcher();
    }
    
    // ========== Directory and Path Management ==========
    
    std::string DownloadManager::getDownloadDirectory() {
        if (!default_download_dir_.empty() && PathUtils::isDirectory(default_download_dir_)) {
            return default_download_dir_;
        }
        
        return PathUtils::getDefaultDownloadDirectory();
    }
    
    bool DownloadManager::setDownloadDirectory(const std::string& directory) {
        std::string normalized = PathUtils::normalizePath(directory);
        
        if (!PathUtils::exists(normalized)) {
            if (!ensureDownloadDirectoryExists(normalized)) {
                return false;
            }
        }
        
        if (!PathUtils::isDirectory(normalized) || !PathUtils::isWritable(normalized)) {
            return false;
        }
        
        default_download_dir_ = normalized;
        return true;
    }
    
    bool DownloadManager::ensureDownloadDirectoryExists(const std::string& directory) {
        return PathUtils::createDirectoriesIfNeeded(directory);
    }
    
    std::vector<std::string> DownloadManager::getPotentialDownloadDirectories() {
        std::vector<std::string> directories;
        
        // Primary download directory
        directories.push_back(PathUtils::getDefaultDownloadDirectory());
        
        // Desktop as fallback
        std::string home = PathUtils::getHomeDirectory();
        directories.push_back(PathUtils::joinPaths({home, "Desktop"}));
        
        // Documents folder
        directories.push_back(PathUtils::joinPaths({home, "Documents"}));
        
        // Current working directory
        directories.push_back(std::filesystem::current_path().string());
        
        // Temp directory
        directories.push_back(PathUtils::getTempDirectory());
        
        // Filter to only existing, writable directories
        std::vector<std::string> valid_directories;
        for (const auto& dir : directories) {
            if (PathUtils::isDirectory(dir) && PathUtils::isWritable(dir)) {
                valid_directories.push_back(dir);
            }
        }
        
        return valid_directories;
    }
    
    // ========== File Detection and Monitoring ==========
    
    std::vector<std::string> DownloadManager::findMatchingFiles(
        const std::string& directory,
        const std::string& pattern) {
        
        return PathUtils::findFilesMatchingPattern(directory, pattern);
    }
    
    bool DownloadManager::fileMatchesPattern(const std::string& filepath, const std::string& pattern) {
        std::string filename = PathUtils::getFileName(filepath);
        
        if (PathUtils::isRegexPattern(pattern)) {
            return PathUtils::matchesRegexPattern(filename, pattern);
        } else {
            return PathUtils::matchesGlobPattern(filename, pattern);
        }
    }
    
    bool DownloadManager::monitorDirectoryForNewFiles(
        const std::string& directory,
        const std::string& pattern,
        int timeout_ms,
        std::function<void(const std::string&)> file_found_callback) {
        
        if (!isNativeFileWatchingAvailable()) {
            return false;
        }
        
        if (!initializePlatformWatcher(directory)) {
            debug_output("Failed to initialize native file watcher");
            return false;
        }
        
        std::atomic<bool> found_file{false};
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms);
        
        auto watcher_thread = std::thread([this, &found_file, file_found_callback, pattern]() {
            processFileSystemEvents([&](const std::string& filepath, const std::string& event_type) {
                if (event_type == "created" || event_type == "modified") {
                    if (fileMatchesPattern(filepath, pattern) && !isBrowserTempFile(filepath)) {
                        file_found_callback(filepath);
                        found_file = true;
                    }
                }
            });
        });
        
        // Wait for file to be found or timeout
        while (!found_file && (std::chrono::steady_clock::now() - start_time < timeout)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Clean up watcher
        cleanupPlatformWatcher();
        if (watcher_thread.joinable()) {
            watcher_thread.join();
        }
        
        return found_file;
    }
    
    bool DownloadManager::isDownloadInProgress(const std::string& filepath) {
        // Check for browser temporary file extensions
        if (isBrowserTempFile(filepath)) {
            return true;
        }
        
        // Check if file size is changing
        size_t size1 = PathUtils::getFileSize(filepath);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        size_t size2 = PathUtils::getFileSize(filepath);
        
        return size1 != size2;
    }
    
    // ========== Download Completion Detection ==========
    
    bool DownloadManager::waitForDownloadCompletion(
        const std::string& filepath,
        int timeout_ms,
        std::function<void(const DownloadProgress&)> progress_callback) {
        
        debug_output("Waiting for download completion: " + filepath);
        
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms);
        
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            // Check if file exists and is readable
            if (!PathUtils::exists(filepath) || !PathUtils::isReadable(filepath)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
            
            // Check if still a temporary file
            if (isBrowserTempFile(filepath)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            
            // Check file size stability
            if (isFileSizeStable(filepath, stability_check_duration_)) {
                // Additional check: make sure file is not zero bytes
                size_t file_size = PathUtils::getFileSize(filepath);
                if (file_size > 0) {
                    debug_output("Download completed: " + filepath + 
                               " (" + PathUtils::formatFileSize(file_size) + ")");
                    return true;
                }
            }
            
            // Report progress if callback provided
            if (progress_callback) {
                DownloadProgress progress;
                progress.filepath = filepath;
                progress.current_size = PathUtils::getFileSize(filepath);
                progress.start_time = std::chrono::system_clock::now(); // FIXED: Use system_clock
                progress.last_update = std::chrono::system_clock::now();
                progress.is_complete = false;
                
                progress_callback(progress);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        debug_output("Download completion timeout: " + filepath);
        return false;
    }
    
    bool DownloadManager::verifyDownloadIntegrity(const std::string& filepath, size_t expected_size) {
        if (!integrity_verification_enabled_) {
            return true;
        }
        
        // Basic checks
        if (!PathUtils::exists(filepath) || !PathUtils::isFile(filepath)) {
            debug_output("Download integrity failed: file doesn't exist");
            return false;
        }
        
        if (!PathUtils::isReadable(filepath)) {
            debug_output("Download integrity failed: file not readable");
            return false;
        }
        
        size_t actual_size = PathUtils::getFileSize(filepath);
        if (actual_size == 0) {
            debug_output("Download integrity failed: zero byte file");
            return false;
        }
        
        // Check expected size if provided
        if (expected_size > 0 && actual_size != expected_size) {
            debug_output("Download integrity failed: size mismatch. Expected: " + 
                        std::to_string(expected_size) + ", Actual: " + std::to_string(actual_size));
            return false;
        }
        
        // Try to open file to ensure it's not corrupted
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            debug_output("Download integrity failed: cannot open file");
            return false;
        }
        
        return true;
    }
    
    bool DownloadManager::isFileSizeStable(
        const std::string& filepath,
        std::chrono::milliseconds stability_duration) {
        
        if (!PathUtils::exists(filepath)) {
            return false;
        }
        
        size_t initial_size = PathUtils::getFileSize(filepath);
        auto start_time = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - start_time < stability_duration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            size_t current_size = PathUtils::getFileSize(filepath);
            if (current_size != initial_size) {
                // Size changed, restart stability check
                initial_size = current_size;
                start_time = std::chrono::steady_clock::now();
            }
        }
        
        return true;
    }
    
    int DownloadManager::getDownloadProgress(const std::string& filepath, size_t expected_size) {
        if (!PathUtils::exists(filepath) || expected_size == 0) {
            return -1;
        }
        
        size_t current_size = PathUtils::getFileSize(filepath);
        return static_cast<int>((current_size * 100) / expected_size);
    }
    
    // ========== Pattern Matching and Filtering ==========
    
    std::string DownloadManager::globToRegex(const std::string& glob_pattern) {
        // Local implementation since PathUtils::globToRegex is private
        std::string regex;
        regex.reserve(glob_pattern.length() * 2);
        
        for (size_t i = 0; i < glob_pattern.length(); ++i) {
            char c = glob_pattern[i];
            switch (c) {
                case '*':
                    regex += ".*";
                    break;
                case '?':
                    regex += ".";
                    break;
                case '.':
                    regex += "\\.";
                    break;
                case '^':
                case '$':
                case '(':
                case ')':
                case '[':
                case ']':
                case '{':
                case '}':
                case '+':
                case '|':
                case '\\':
                    regex += '\\';
                    regex += c;
                    break;
                default:
                    regex += c;
                    break;
            }
        }
        
        return regex;
    }
    
    bool DownloadManager::isGlobPattern(const std::string& pattern) {
        return PathUtils::isGlobPattern(pattern);
    }
    
    bool DownloadManager::isRegexPattern(const std::string& pattern) {
        return PathUtils::isRegexPattern(pattern);
    }
    
    std::string DownloadManager::getMostRecentMatchingFile(
        const std::string& directory,
        const std::string& pattern) {
        
        auto matching_files = findMatchingFiles(directory, pattern);
        if (matching_files.empty()) {
            return "";
        }
        
        // Sort by modification time (most recent first)
        std::sort(matching_files.begin(), matching_files.end(),
            [this](const std::string& a, const std::string& b) {
                return getFileCreationTime(a) > getFileCreationTime(b);
            });
        
        return matching_files[0];
    }
    
    // ========== File System Watching ==========
    
    bool DownloadManager::startNativeFileWatcher(
        const std::string& directory,
        std::function<void(const std::string&, const std::string&)> change_callback) {
        
        return initializePlatformWatcher(directory);
    }
    
    void DownloadManager::stopNativeFileWatcher() {
        cleanupPlatformWatcher();
    }
    
    bool DownloadManager::isNativeFileWatchingAvailable() {
        #if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
            return true;
        #else
            return false;
        #endif
    }
    
    bool DownloadManager::startPollingFileMonitor(
        const std::string& directory,
        const std::string& pattern,
        int poll_interval_ms,
        int timeout_ms,
        std::function<void(const std::string&)> file_found_callback) {
        
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms);
        
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            auto matching_files = findMatchingFiles(directory, pattern);
            for (const auto& filepath : matching_files) {
                if (!isBrowserTempFile(filepath)) {
                    file_found_callback(filepath);
                    return true;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval_ms));
        }
        
        return false;
    }
    
    // ========== Browser Integration ==========
    
    std::vector<std::string> DownloadManager::getBrowserDownloadPatterns(const std::string& filename) {
        std::vector<std::string> patterns;
        
        // Exact filename
        patterns.push_back(filename);
        
        // Chrome patterns
        patterns.push_back(filename + ".crdownload");
        
        // Firefox patterns  
        patterns.push_back(filename + ".part");
        
        // Safari patterns
        patterns.push_back(filename + ".download");
        
        // Generic patterns
        patterns.push_back("*" + filename + "*");
        
        return patterns;
    }
    
    std::string DownloadManager::resolveBrowserTempFile(const std::string& temp_filepath) {
        std::string filename = PathUtils::getFileName(temp_filepath);
        
        // Remove common browser temp extensions using endsWith helper
        if (endsWith(filename, ".crdownload")) {
            return temp_filepath.substr(0, temp_filepath.length() - 11);
        }
        if (endsWith(filename, ".part")) {
            return temp_filepath.substr(0, temp_filepath.length() - 5);
        }
        if (endsWith(filename, ".download")) {
            return temp_filepath.substr(0, temp_filepath.length() - 9);
        }
        
        return temp_filepath;
    }
    
    bool DownloadManager::isBrowserTempFile(const std::string& filepath) {
        std::string filename = PathUtils::getFileName(filepath);
        
        // Check for common browser temporary file extensions using helpers
        return endsWith(filename, ".crdownload") ||  // Chrome
               endsWith(filename, ".part") ||        // Firefox
               endsWith(filename, ".download") ||    // Safari
               endsWith(filename, ".tmp") ||         // Generic temp
               startsWith(filename, "~");            // Generic temp prefix
    }
    
    bool DownloadManager::waitForBrowserWriteCompletion(const std::string& filepath, int timeout_ms) {
        return waitForDownloadCompletion(filepath, timeout_ms);
    }
    
    // ========== Configuration and Options ==========
    
    void DownloadManager::setDefaultTimeout(int timeout_ms) {
        default_timeout_ms_ = timeout_ms;
    }
    
    void DownloadManager::setStabilityCheckDuration(std::chrono::milliseconds duration) {
        stability_check_duration_ = duration;
    }
    
    void DownloadManager::setIntegrityVerificationEnabled(bool enabled) {
        integrity_verification_enabled_ = enabled;
    }
    
    void DownloadManager::setPollingInterval(int interval_ms) {
        polling_interval_ms_ = interval_ms;
    }
    
    // ========== Utility Methods ==========
    
    std::string DownloadManager::downloadResultToString(DownloadResult result) {
        switch (result) {
            case DownloadResult::SUCCESS: return "SUCCESS";
            case DownloadResult::TIMEOUT: return "TIMEOUT";
            case DownloadResult::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
            case DownloadResult::INTEGRITY_CHECK_FAILED: return "INTEGRITY_CHECK_FAILED";
            case DownloadResult::PERMISSION_DENIED: return "PERMISSION_DENIED";
            case DownloadResult::DIRECTORY_NOT_FOUND: return "DIRECTORY_NOT_FOUND";
            case DownloadResult::PATTERN_MATCH_FAILED: return "PATTERN_MATCH_FAILED";
            default: return "UNKNOWN";
        }
    }
    
    DownloadManager::DownloadStats DownloadManager::getDownloadStatistics() {
        DownloadStats stats;
        stats.active_downloads = active_downloads_.load();
        stats.completed_downloads = completed_downloads_.load();
        stats.failed_downloads = failed_downloads_.load();
        stats.average_completion_time = std::chrono::milliseconds(5000); // Placeholder
        
        return stats;
    }
    
    std::string DownloadManager::getErrorMessage(DownloadResult result, const std::string& pattern) {
        switch (result) {
            case DownloadResult::SUCCESS:
                return "Download completed successfully";
            case DownloadResult::TIMEOUT:
                return "Download timeout waiting for: " + pattern;
            case DownloadResult::FILE_NOT_FOUND:
                return "No file matching pattern found: " + pattern;
            case DownloadResult::INTEGRITY_CHECK_FAILED:
                return "Download integrity check failed for: " + pattern;
            case DownloadResult::PERMISSION_DENIED:
                return "Permission denied accessing download directory";
            case DownloadResult::DIRECTORY_NOT_FOUND:
                return "Download directory not found";
            case DownloadResult::PATTERN_MATCH_FAILED:
                return "Pattern matching failed for: " + pattern;
            default:
                return "Unknown download error";
        }
    }
    
    bool DownloadManager::hasInsufficientDiskSpace(const std::string& directory, size_t required_bytes) {
        try {
            auto space_info = std::filesystem::space(directory);
            return space_info.available < required_bytes;
        } catch (const std::exception& e) {
            return false; // Assume sufficient space if we can't check
        }
    }
    
    // ========== Advanced Features ==========
    
    void DownloadManager::setDownloadCompletionHook(std::function<void(const std::string&)> hook) {
        completion_hook_ = hook;
    }
    
    void DownloadManager::setProgressNotificationCallback(
        std::function<void(const std::string&, int)> progress_callback) {
        progress_callback_ = progress_callback;
    }
    
    void DownloadManager::createDownloadManifest(
        const std::vector<std::string>& expected_files,
        const std::string& manifest_path) {
        
        std::ofstream manifest_file(manifest_path);
        if (manifest_file.is_open()) {
            for (const auto& filename : expected_files) {
                manifest_file << filename << ":pending" << std::endl;
            }
        }
    }
    
    bool DownloadManager::isDownloadManifestComplete(const std::string& manifest_path) {
        std::ifstream manifest_file(manifest_path);
        if (!manifest_file.is_open()) {
            return false;
        }
        
        std::string line;
        while (std::getline(manifest_file, line)) {
            if (line.find(":pending") != std::string::npos) {
                return false; // Found pending download
            }
        }
        
        return true; // All downloads complete
    }
    
    // ========== Platform-Specific Implementation ==========
    
    bool DownloadManager::initializePlatformWatcher(const std::string& directory) {
        #ifdef _WIN32
            // Windows implementation using ReadDirectoryChangesW
            directory_handle_ = CreateFileA(
                directory.c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                NULL
            );
            
            return directory_handle_ != INVALID_HANDLE_VALUE;
            
        #elif defined(__linux__)
            // Linux implementation using inotify
            inotify_fd_ = inotify_init1(IN_NONBLOCK);
            if (inotify_fd_ == -1) {
                return false;
            }
            
            watch_descriptor_ = inotify_add_watch(
                inotify_fd_, 
                directory.c_str(),
                IN_CREATE | IN_MODIFY | IN_MOVED_TO
            );
            
            return watch_descriptor_ != -1;
            
        #elif defined(__APPLE__)
            // macOS implementation using kqueue
            kqueue_fd_ = kqueue();
            if (kqueue_fd_ == -1) {
                return false;
            }
            
            int dir_fd = open(directory.c_str(), O_RDONLY);
            if (dir_fd == -1) {
                close(kqueue_fd_);
                return false;
            }
            
            struct kevent change;
            EV_SET(&change, dir_fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_ONESHOT,
                   NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB, 0, NULL);
                   
            return kevent(kqueue_fd_, &change, 1, NULL, 0, NULL) != -1;
            
        #else
            return false; // Platform not supported
        #endif
    }
    
    void DownloadManager::cleanupPlatformWatcher() {
        #ifdef _WIN32
            if (directory_handle_ && directory_handle_ != INVALID_HANDLE_VALUE) {
                CloseHandle(directory_handle_);
                directory_handle_ = nullptr;
            }
            if (completion_port_ && completion_port_ != INVALID_HANDLE_VALUE) {
                CloseHandle(completion_port_);
                completion_port_ = nullptr;
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
            if (kqueue_fd_ != -1) {
                close(kqueue_fd_);
                kqueue_fd_ = -1;
            }
        #endif
    }
    
    void DownloadManager::processFileSystemEvents(
        std::function<void(const std::string&, const std::string&)> callback) {
        
        #ifdef _WIN32
            // Windows event processing implementation
            DWORD bytes_returned;
            char buffer[1024];
            
            while (monitoring_active_) {
                if (ReadDirectoryChangesW(
                    directory_handle_,
                    buffer,
                    sizeof(buffer),
                    FALSE,
                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE,
                    &bytes_returned,
                    NULL,
                    NULL)) {
                    
                    // Process file change notifications
                    // This is a simplified implementation
                    callback("file_changed", "created");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
        #elif defined(__linux__)
            // Linux inotify event processing
            char buffer[1024];
            
            while (monitoring_active_) {
                struct pollfd pfd = {inotify_fd_, POLLIN, 0};
                int poll_result = poll(&pfd, 1, 100); // 100ms timeout
                
                if (poll_result > 0) {
                    ssize_t length = read(inotify_fd_, buffer, sizeof(buffer));
                    if (length > 0) {
                        struct inotify_event* event = (struct inotify_event*)buffer;
                        if (event->len > 0) {
                            std::string filename = event->name;
                            std::string event_type = "created";
                            
                            if (event->mask & IN_CREATE) event_type = "created";
                            else if (event->mask & IN_MODIFY) event_type = "modified";
                            else if (event->mask & IN_MOVED_TO) event_type = "moved";
                            
                            callback(filename, event_type);
                        }
                    }
                }
            }
            
        #elif defined(__APPLE__)
            // macOS kqueue event processing
            struct kevent events[10];
            
            while (monitoring_active_) {
                struct timespec timeout = {0, 100000000}; // 100ms
                int event_count = kevent(kqueue_fd_, NULL, 0, events, 10, &timeout);
                
                for (int i = 0; i < event_count; ++i) {
                    if (events[i].filter == EVFILT_VNODE) {
                        callback("directory_changed", "modified");
                    }
                }
            }
        #endif
    }
    
    std::chrono::system_clock::time_point DownloadManager::getFileCreationTime(const std::string& filepath) {
        return PathUtils::getModificationTime(filepath);
    }
    
    void DownloadManager::updateDownloadStats(bool completed, bool failed) {
        if (completed) {
            completed_downloads_++;
            if (active_downloads_ > 0) {
                active_downloads_--;
            }
        } else if (failed) {
            failed_downloads_++;
            if (active_downloads_ > 0) {
                active_downloads_--;
            }
        } else {
            active_downloads_++;
        }
    }
    
    // ========== Placeholder implementations for missing methods ==========
    
    void DownloadManager::cleanupDownloadArtifacts(const std::string& directory) {
        // Implementation for cleanup of download artifacts
    }
    
    bool DownloadManager::moveDownloadToDestination(
        const std::string& source_path,
        const std::string& destination_path) {
        return PathUtils::moveFile(source_path, destination_path);
    }
    
    FileInfo DownloadManager::getDownloadInfo(const std::string& filepath) {
        return FileInfo::create(filepath);
    }
    
    bool DownloadManager::validateDownloadedFile(const std::string& filepath) {
        return verifyDownloadIntegrity(filepath, 0);
    }
    
    std::string DownloadManager::normalizePath(const std::string& path) {
        return PathUtils::normalizePath(path);
    }
    
    bool DownloadManager::appearsCompletelyDownloaded(const std::string& filepath) {
        return !isBrowserTempFile(filepath) && isFileSizeStable(filepath);
    }
    
    bool DownloadManager::monitorSingleFileCompletion(
        const std::string& filepath,
        int timeout_ms,
        std::function<void(const DownloadProgress&)> progress_callback) {
        return waitForDownloadCompletion(filepath, timeout_ms, progress_callback);
    }
    
    std::string DownloadManager::generateMatchingRegex(const std::string& pattern) {
        if (isRegexPattern(pattern)) {
            return pattern.substr(1, pattern.length() - 2); // Remove /pattern/ wrapper
        }
        return globToRegex(pattern);
    }
    
    bool DownloadManager::isDirectoryActivelyChanging(const std::string& directory) {
        // Simple implementation - check if any files are changing
        try {
            auto files = PathUtils::findFilesMatchingPattern(directory, "*");
            for (const auto& file : files) {
                if (isDownloadInProgress(file)) {
                    return true;
                }
            }
        } catch (...) {
            // Ignore errors
        }
        return false;
    }
    
} // namespace FileOps
